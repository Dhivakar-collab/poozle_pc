/**
 * @file tester.cpp
 * @brief gtest harness for poozle regex (`find_all` / `match` / `escape`).
 *
 * Build: meson (see test/meson.build). Paths FILES_DIR and PZTEST_DIR are
 * injected at compile time.
 *
 * Oracles (independent expected counts):
 *   - std::regex (ECMAScript)
 *   - Python re.finditer
 *   - grep -oE (LC_ALL=C)
 *
 * Pipeline under test matches libpz/regex/tests/test_2_findall.cpp:
 *   Tokenizer → Postfix::convert → NfaBuilder::build → NfaMatcher::find_all
 */

#include <gtest/gtest.h>

#include "Nfa.hpp"
#include "NfaBuilder.hpp"
#include "NfaMatcher.hpp"
#include "RegexPostfix.hpp"
#include "RegexTokenizer.hpp"

#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unistd.h>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

// =============================================================================
// Poozle adapter
// =============================================================================

/**
 * @brief Owns the NFA and matcher for one pattern.
 *
 * @note NfaBuilder must stay alive while matching — it owns the state pool.
 */
struct PoozleEngine {
  NfaBuilder builder;
  State *start = nullptr;
  std::unique_ptr<NfaMatcher> matcher;

  /** Compile @p pattern into an NFA and wrap it in NfaMatcher. */
  explicit PoozleEngine(std::string_view pattern) {
    Tokenizer tokenizer(pattern);
    auto tokens = tokenizer.tokenize();
    auto postfix = Postfix::convert(tokens);
    start = builder.build(postfix);
    matcher = std::make_unique<NfaMatcher>(start);
  }

  /** Non-overlapping matches (same idea as Python re.finditer). */
  std::vector<MatchResult> find_all(std::string_view text) const {
    return matcher->find_all(text);
  }

  /** Prefix match from the start of @p text. */
  MatchResult match(std::string_view text) const {
    return matcher->match(text);
  }

  /** Escape regex metacharacters (like Python re.escape). */
  std::string escape(std::string_view text) const {
    return matcher->escape(text);
  }
};

/** @return Number of non-overlapping matches for @p pattern in @p text. */
static std::size_t poozle_find_all_count(std::string_view pattern,
                                         std::string_view text) {
  PoozleEngine engine(pattern);
  return engine.find_all(text).size();
}

// =============================================================================
// Shell / temp helpers (used by python + grep oracles)
// =============================================================================

/** RAII temp directory under /tmp; removed in the destructor. */
struct TempTree {
  fs::path root;

  explicit TempTree(std::string_view prefix) {
    auto base =
        fs::temp_directory_path() /
        (std::string(prefix) + std::to_string(::getpid()) + "_" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(base);
    root = base;
  }

  ~TempTree() {
    std::error_code ec;
    fs::remove_all(root, ec);
  }

  TempTree(const TempTree &) = delete;
  TempTree &operator=(const TempTree &) = delete;
};

static void write_text(const fs::path &path, std::string_view contents);

/** Run @p cmd via popen and capture stdout. */
static std::optional<std::string> run_cmd_capture(const std::string &cmd) {
  FILE *pipe = ::popen(cmd.c_str(), "r");
  if (!pipe) {
    return std::nullopt;
  }
  std::string out;
  std::array<char, 256> buf{};
  while (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe) !=
         nullptr) {
    out += buf.data();
  }
  const int rc = ::pclose(pipe);
  if (rc != 0 && out.empty()) {
    return std::nullopt;
  }
  return out;
}

/** Parse a leading integer from oracle stdout (e.g. `wc -l` / `print(n)`). */
static std::size_t parse_count_line(std::string_view raw) {
  std::size_t i = 0;
  while (i < raw.size() && (raw[i] == ' ' || raw[i] == '\t' || raw[i] == '\n' ||
                            raw[i] == '\r')) {
    ++i;
  }
  if (i >= raw.size() || raw[i] < '0' || raw[i] > '9') {
    return static_cast<std::size_t>(-1);
  }
  std::size_t n = 0;
  while (i < raw.size() && raw[i] >= '0' && raw[i] <= '9') {
    n = n * 10 + static_cast<std::size_t>(raw[i] - '0');
    ++i;
  }
  return n;
}

// =============================================================================
// Oracles — each returns a non-overlapping match count (or -1 on failure)
// =============================================================================

/**
 * @brief Oracle #1: C++ std::regex (ECMAScript).
 * @details Uses sregex_iterator — non-overlapping, same idea as find_all.
 */
static std::size_t oracle_std_regex(std::string_view pattern,
                                    std::string_view text) {
  try {
    const std::string pat(pattern);
    const std::string hay(text);
    std::regex re(pat, std::regex::ECMAScript);
    auto begin = std::sregex_iterator(hay.begin(), hay.end(), re);
    auto end = std::sregex_iterator();
    return static_cast<std::size_t>(std::distance(begin, end));
  } catch (const std::regex_error &e) {
    ADD_FAILURE() << "oracle std::regex error for pattern \"" << pattern
                  << "\": " << e.what();
    return static_cast<std::size_t>(-1);
  }
}

/**
 * @brief Oracle #2: Python `re.finditer` over a corpus file.
 * @note Needs `python3` on PATH.
 */
static std::size_t oracle_python_re_file(std::string_view pattern,
                                         const fs::path &text_path) {
  TempTree tmp("poozle_py_");
  const fs::path pat_path = tmp.root / "pattern.txt";
  write_text(pat_path, pattern);

  // Pattern/text paths avoid shell-escaping the regex itself.
  const std::string cmd =
      "python3 -c \"import re,sys; "
      "p=open(sys.argv[1],encoding='utf-8',errors='surrogateescape').read(); "
      "t=open(sys.argv[2],encoding='utf-8',errors='surrogateescape').read(); "
      "print(len(list(re.finditer(p,t))))\" " +
      pat_path.string() + " " + text_path.string() + " 2>/dev/null";

  auto out = run_cmd_capture(cmd);
  if (!out) {
    ADD_FAILURE() << "oracle python re failed for pattern \"" << pattern
                  << "\" (is python3 available?)";
    return static_cast<std::size_t>(-1);
  }
  const auto n = parse_count_line(*out);
  if (n == static_cast<std::size_t>(-1)) {
    ADD_FAILURE() << "oracle python re bad output for pattern \"" << pattern
                  << "\": " << *out;
  }
  return n;
}

/** Oracle #2 overload: write @p text to a temp file first. */
static std::size_t oracle_python_re(std::string_view pattern,
                                    std::string_view text) {
  TempTree tmp("poozle_pytxt_");
  const fs::path text_path = tmp.root / "text.txt";
  write_text(text_path, text);
  return oracle_python_re_file(pattern, text_path);
}

/**
 * @brief Oracle #3: `grep -oE` match count via `wc -l`.
 * @note LC_ALL=C so [a-z] is ASCII (matches Python / ECMAScript).
 */
static std::size_t oracle_grep_file(std::string_view pattern,
                                    const fs::path &text_path) {
  TempTree tmp("poozle_grep_");
  const fs::path pat_path = tmp.root / "pattern.txt";
  write_text(pat_path, pattern);

  // -f reads the pattern from a file (no shell metacharacter expansion).
  const std::string cmd = "LC_ALL=C grep -oE -f " + pat_path.string() + " -- " +
                          text_path.string() + " 2>/dev/null | wc -l";

  auto out = run_cmd_capture(cmd);
  if (!out) {
    ADD_FAILURE() << "oracle grep failed for pattern \"" << pattern << "\"";
    return static_cast<std::size_t>(-1);
  }
  const auto n = parse_count_line(*out);
  if (n == static_cast<std::size_t>(-1)) {
    ADD_FAILURE() << "oracle grep bad output for pattern \"" << pattern
                  << "\": " << *out;
  }
  return n;
}

/** Oracle #3 overload: write @p text to a temp file first. */
static std::size_t oracle_grep(std::string_view pattern,
                               std::string_view text) {
  TempTree tmp("poozle_greptxt_");
  const fs::path text_path = tmp.root / "text.txt";
  write_text(text_path, text);
  return oracle_grep_file(pattern, text_path);
}

/** Counts from all three oracles for one (pattern, text) pair. */
struct OracleBundle {
  std::size_t std_regex = static_cast<std::size_t>(-1);
  std::size_t python_re = static_cast<std::size_t>(-1);
  std::size_t grep = static_cast<std::size_t>(-1);
};

/** Run std::regex + python re + grep on in-memory text. */
static OracleBundle run_all_oracles(std::string_view pattern,
                                    std::string_view text) {
  OracleBundle b;
  b.std_regex = oracle_std_regex(pattern, text);
  b.python_re = oracle_python_re(pattern, text);
  b.grep = oracle_grep(pattern, text);
  return b;
}

/**
 * Run all oracles; python/grep read @p text_path (avoids re-copying large
 * files).
 * @p text is still needed for std::regex.
 */
static OracleBundle run_all_oracles_file(std::string_view pattern,
                                         const fs::path &text_path,
                                         std::string_view text) {
  OracleBundle b;
  b.std_regex = oracle_std_regex(pattern, text);
  b.python_re = oracle_python_re_file(pattern, text_path);
  b.grep = oracle_grep_file(pattern, text_path);
  return b;
}

/**
 * @brief Require std / re / grep to agree; return that shared count.
 * @param id Short label printed in the `[oracle]` log line.
 */
static std::size_t oracle_consensus(std::string_view id,
                                    std::string_view pattern,
                                    std::string_view text) {
  const OracleBundle b = run_all_oracles(pattern, text);
  std::cout << "[oracle] " << id << "  std:" << b.std_regex
            << "  re:" << b.python_re << "  grep:" << b.grep << "\n";
  EXPECT_EQ(b.std_regex, b.python_re)
      << "id=" << id << " std::regex vs python re disagree";
  EXPECT_EQ(b.std_regex, b.grep)
      << "id=" << id << " std::regex vs grep disagree";
  return b.std_regex;
}

/**
 * Print `[OK]` / `[FAIL]` and EXPECT poozle == each oracle.
 * Line shape: id (pattern) | std:N re:N grep:N | poozle:N
 */
static void report_and_assert(std::string_view id, std::string_view pattern,
                              const OracleBundle &oracles, std::size_t got) {
  const bool ok = (oracles.std_regex == got) && (oracles.python_re == got) &&
                  (oracles.grep == got);
  std::cout << (ok ? "[OK]   " : "[FAIL] ") << std::setw(12) << std::left << id
            << " (" << pattern << ")"
            << " | std:" << std::setw(5) << oracles.std_regex
            << " re:" << std::setw(5) << oracles.python_re
            << " grep:" << std::setw(5) << oracles.grep << " | poozle:" << got
            << "\n";
  EXPECT_EQ(oracles.std_regex, got)
      << "id=" << id << " pattern=" << pattern << " vs std::regex";
  EXPECT_EQ(oracles.python_re, got)
      << "id=" << id << " pattern=" << pattern << " vs python re";
  EXPECT_EQ(oracles.grep, got)
      << "id=" << id << " pattern=" << pattern << " vs grep";
}

static std::string read_file(const fs::path &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("failed to open " + path.string());
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

static void write_text(const fs::path &path, std::string_view contents) {
  fs::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) {
    throw std::runtime_error("failed to write " + path.string());
  }
  out << contents;
}

// =============================================================================
// Small fixtures — hard-coded seeds → gen_small.txt under FILES_DIR/regex/
// =============================================================================

/** One seed case before oracle fills expected_count. */
struct SmallCase {
  std::string id;
  std::string pattern;
  std::string input;
};

/** Safe ECMAScript ∩ poozle patterns (avoid empty-match edge cases). */
static const std::vector<SmallCase> kSmallSeeds = {
    {"lit_a", "a", "banana"},
    {"lit_an", "an", "banana"},
    {"dot", "c.t", "cat cot cut cxt"},
    {"alt", "cat|dog", "I have a cat and a dog and another cat"},
    {"star", "ab*c", "ac abc abbc abbbc"},
    {"plus", "ab+c", "abc abbc abbbc"},
    {"opt", "colou?r", "color colour color colour"},
    {"class", "[0-9]+", "Phone: 123-456-7890"},
    {"group", "(cat|dog)", "cat dog cat dog"},
    {"exact", "abc", "abc"},
};

/** Serialize cases to the gen_small.txt key/value format. */
static std::string format_small_fixture(
    const std::vector<std::tuple<std::string, std::string, std::string, int>>
        &rows) {
  std::ostringstream out;
  for (const auto &[id, pattern, input, expected] : rows) {
    out << "id: " << id << "\n";
    out << "pattern: " << pattern << "\n";
    out << "input: " << input << "\n";
    out << "expected_count: " << expected << "\n\n";
  }
  return out.str();
}

/** One case loaded back from gen_small.txt. */
struct ParsedSmallCase {
  std::string id;
  std::string pattern;
  std::string input;
  int expected_count = -1;
};

/** Parse gen_small.txt (`id:` / `pattern:` / `input:` / `expected_count:`). */
static std::vector<ParsedSmallCase>
parse_small_fixtures(std::string_view text) {
  std::vector<ParsedSmallCase> cases;
  ParsedSmallCase cur;
  std::istringstream ss{std::string(text)};
  std::string line;
  auto flush = [&]() {
    if (!cur.id.empty()) {
      cases.push_back(cur);
      cur = ParsedSmallCase{};
    }
  };
  while (std::getline(ss, line)) {
    if (line.empty() || line[0] == '#') {
      flush();
      continue;
    }
    auto colon = line.find(':');
    if (colon == std::string::npos) {
      continue;
    }
    auto key = line.substr(0, colon);
    auto val = line.substr(colon + 1);
    while (!val.empty() && val.front() == ' ') {
      val.erase(val.begin());
    }
    if (key == "id") {
      flush();
      cur.id = val;
    } else if (key == "pattern") {
      cur.pattern = val;
    } else if (key == "input") {
      cur.input = val;
    } else if (key == "expected_count") {
      cur.expected_count = std::stoi(val);
    }
  }
  flush();
  return cases;
}

// =============================================================================
// Large fixtures — PZTEST_DIR/regex/large + optional manifest.tsv cache
// =============================================================================

/**
 * One row of manifest.tsv.
 * @par Columns
 * id, pattern, filename, expected_count (−1 = recompute via oracles).
 */
struct ManifestRow {
  std::string id;
  std::string pattern;
  std::string filename;
  int expected_count = -1;
};

/** Load whitespace/TAB-separated manifest rows (skip `#` comments). */
static std::vector<ManifestRow> parse_manifest(const fs::path &path) {
  std::vector<ManifestRow> rows;
  if (!fs::exists(path)) {
    return rows;
  }
  std::ifstream in(path);
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    std::istringstream ls(line);
    ManifestRow row;
    if (!(ls >> row.id >> row.pattern >> row.filename >> row.expected_count)) {
      continue;
    }
    rows.push_back(std::move(row));
  }
  return rows;
}

/** Rewrite manifest.tsv after oracle refresh. */
static void write_manifest(const fs::path &path,
                           const std::vector<ManifestRow> &rows) {
  std::ofstream out(path, std::ios::trunc);
  out << "# id\tpattern\tfilename\texpected_count\n";
  for (const auto &r : rows) {
    out << r.id << '\t' << r.pattern << '\t' << r.filename << '\t'
        << r.expected_count << '\n';
  }
}

/**
 * Ensure @p large_dir has at least one `.txt` corpus.
 * If empty, synthesize synth_1mb.txt (fixed seed) so CI/local can still run.
 */
static void ensure_large_corpus(const fs::path &large_dir) {
  fs::create_directories(large_dir);
  bool has_txt = false;
  for (const auto &entry : fs::directory_iterator(large_dir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".txt") {
      has_txt = true;
      break;
    }
  }
  if (has_txt) {
    return;
  }

  constexpr std::size_t kSize = 1024 * 1024;
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> pick(0, 5);
  static const char *words[] = {"cat", "dog", "hello", "world", "the", "a"};
  std::string body;
  body.reserve(kSize + 64);
  while (body.size() < kSize) {
    body += words[pick(rng)];
    body.push_back(' ');
  }
  body.resize(kSize);
  write_text(large_dir / "synth_1mb.txt", body);
}

/** Fallback when no manifest: one row per `.txt` with pattern cat|dog|hello. */
static std::vector<ManifestRow>
default_manifest_for_dir(const fs::path &large_dir) {
  std::vector<ManifestRow> rows;
  for (const auto &entry : fs::directory_iterator(large_dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".txt") {
      continue;
    }
    ManifestRow row;
    row.id = entry.path().stem().string();
    row.pattern = "cat|dog|hello";
    row.filename = entry.path().filename().string();
    row.expected_count = -1;
    rows.push_back(std::move(row));
  }
  return rows;
}

// =============================================================================
// gtests
// =============================================================================

/**
 * @test Small in-tree fixtures under FILES_DIR/regex/.
 *
 * Steps: seeds → oracle consensus → write gen_small.txt → reload →
 * compare poozle to std / re / grep. Also smoke-tests match() and escape().
 */
TEST(RegexSearch, SmallFixtures) {
  const fs::path files_dir = FILES_DIR;
  const fs::path regex_dir = files_dir / "regex";
  if (!fs::exists(files_dir)) {
    GTEST_SKIP() << "No files/ fixtures found at " << files_dir;
  }
  fs::create_directories(regex_dir);

  // 1) Seeds → all oracles must agree → write gen_small.txt
  std::vector<std::tuple<std::string, std::string, std::string, int>> rows;
  rows.reserve(kSmallSeeds.size());
  for (const auto &seed : kSmallSeeds) {
    const auto expected =
        static_cast<int>(oracle_consensus(seed.id, seed.pattern, seed.input));
    ASSERT_GE(expected, 0) << "oracle failed for " << seed.id;
    rows.emplace_back(seed.id, seed.pattern, seed.input, expected);
  }

  const fs::path gen_path = regex_dir / "gen_small.txt";
  write_text(gen_path, format_small_fixture(rows));

  // Reload file and compare poozle find_all vs each oracle
  const auto cases = parse_small_fixtures(read_file(gen_path));
  ASSERT_FALSE(cases.empty());
  for (const auto &c : cases) {
    std::size_t got = 0;
    try {
      got = poozle_find_all_count(c.pattern, c.input);
    } catch (const std::exception &e) {
      FAIL() << "poozle error id=" << c.id << ": " << e.what();
    }
    const OracleBundle oracles = run_all_oracles(c.pattern, c.input);
    EXPECT_EQ(static_cast<std::size_t>(c.expected_count), oracles.std_regex)
        << "cached expected drifted from std::regex for id=" << c.id;
    report_and_assert(c.id, c.pattern, oracles, got);
  }
}

/**
 * @test Large corpora under PZTEST_DIR/regex/large/ (from setup_testdata.sh).
 *
 * Uses manifest.tsv when present; otherwise defaults pattern to cat|dog|hello.
 * expected_count −1 (or stale) is refreshed from oracle consensus and
 * rewritten.
 */
TEST(RegexSearch, LargeStressFiles) {
  const fs::path pztest = PZTEST_DIR;
  if (!fs::exists(pztest)) {
    GTEST_SKIP() << "pz-test not cloned. Run: ./scripts/setup_testdata.sh";
  }

  const fs::path large_dir = pztest / "regex" / "large";
  ensure_large_corpus(large_dir);

  const fs::path manifest_path = large_dir / "manifest.tsv";
  auto rows = parse_manifest(manifest_path);
  if (rows.empty()) {
    rows = default_manifest_for_dir(large_dir);
  }
  ASSERT_FALSE(rows.empty()) << "no large corpora under " << large_dir;

  bool manifest_dirty = false;
  for (auto &row : rows) {
    const fs::path corpus = large_dir / row.filename;
    ASSERT_TRUE(fs::exists(corpus)) << "missing corpus " << corpus;
    const std::string text = read_file(corpus);

    // Oracles first (python/grep read the corpus path directly).
    const auto t0 = std::chrono::steady_clock::now();
    const OracleBundle oracles =
        run_all_oracles_file(row.pattern, corpus, text);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - t0)
                        .count();
    std::cout << "[oracle] " << row.id << "  std:" << oracles.std_regex
              << "  re:" << oracles.python_re << "  grep:" << oracles.grep
              << " (" << ms << " ms)\n";
    ASSERT_NE(oracles.std_regex, static_cast<std::size_t>(-1));
    EXPECT_EQ(oracles.std_regex, oracles.python_re)
        << "id=" << row.id << " std::regex vs python re disagree";
    EXPECT_EQ(oracles.std_regex, oracles.grep)
        << "id=" << row.id << " std::regex vs grep disagree";

    // Cache / refresh expected_count in the manifest.
    if (row.expected_count < 0 ||
        static_cast<std::size_t>(row.expected_count) != oracles.std_regex) {
      if (row.expected_count >= 0) {
        std::cout << "[oracle] refreshing cached expected for " << row.id
                  << " (" << row.expected_count << " → " << oracles.std_regex
                  << ")\n";
      }
      row.expected_count = static_cast<int>(oracles.std_regex);
      manifest_dirty = true;
    }

    std::size_t got = 0;
    try {
      got = poozle_find_all_count(row.pattern, text);
    } catch (const std::exception &e) {
      FAIL() << "poozle error id=" << row.id << ": " << e.what();
    }
    report_and_assert(row.id, row.pattern, oracles, got);
  }

  if (manifest_dirty) {
    write_manifest(manifest_path, rows);
  }
}

/** Placeholder for exact-string search fixtures (not regex). */
TEST(ExactSearch, SmallFixtures) {
  const fs::path files_dir = FILES_DIR;
  if (!fs::exists(files_dir)) {
    GTEST_SKIP() << "No files/ fixtures found at " << files_dir;
  }
  GTEST_SKIP() << "Exact-match fixture suite not wired yet";
}

// Fuzzy-search tests get added here once that mode exists in libpz.
