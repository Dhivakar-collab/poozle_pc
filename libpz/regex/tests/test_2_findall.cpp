#include "NfaMatcher.hpp"
#include "nfa_builder.hpp"
#include "postfix.hpp"
#include "tokenizer.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace std::chrono;

// Test statistics
int total_tests = 0;
int passed_tests = 0;
int failed_tests = 0;
long long total_time_us = 0;

// Helper function to print match results
void print_matches(const string &pattern, const string &text,
                   const vector<MatchResult> &matches) {
  cout << "Pattern: \"" << pattern << "\"" << endl;
  cout << "Text:    \"" << text << "\"" << endl;
  cout << "Found " << matches.size() << " match(es):" << endl;

  for (size_t i = 0; i < matches.size(); i++) {
    const auto &m = matches[i];
    string matched_text = text.substr(m.start_pos, m.end_pos - m.start_pos);
    cout << "  Match " << i + 1 << ": [" << m.start_pos << "," << m.end_pos
         << ") = \"" << matched_text << "\"";

    // Print capture groups if any
    if (!m.captures.empty()) {
      cout << " | Groups: ";
      for (size_t j = 0; j < m.captures.size(); j++) {
        if (m.captures[j].first >= 0 && m.captures[j].second >= 0) {
          string group_text = text.substr(
              m.captures[j].first, m.captures[j].second - m.captures[j].first);
          cout << j << "=\"" << group_text << "\" ";
        }
      }
    }
    cout << endl;
  }
  cout << endl;
}

// Test function
void test_find_all(const string &pattern, const string &text,
                   bool verbose = true) {
  total_tests++;
  try {
    auto start_time = high_resolution_clock::now();

    Tokenizer tokenizer(pattern);
    auto tokens = tokenizer.tokenize();
    auto postfix = PostfixConverter::convert(tokens);
    NfaBuilder builder;
    State *start = builder.build(postfix);
    NfaMatcher matcher(start);

    vector<MatchResult> matches = matcher.find_all(text);

    auto end_time = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end_time - start_time);
    total_time_us += duration.count();

    passed_tests++;

    if (verbose) {
      print_matches(pattern, text, matches);
      cout << "Execution time: " << duration.count() << " µs" << endl << endl;
    }

  } catch (const exception &e) {
    failed_tests++;
    cout << "ERROR: Pattern \"" << pattern << "\" - " << e.what() << endl
         << endl;
  }
}

// Test with expected match count
void test_find_all_expected(const string &pattern, const string &text,
                            int expected_count, const string &test_name = "") {
  total_tests++;
  try {
    auto start_time = high_resolution_clock::now();

    Tokenizer tokenizer(pattern);
    auto tokens = tokenizer.tokenize();
    auto postfix = PostfixConverter::convert(tokens);
    NfaBuilder builder;
    State *start = builder.build(postfix);
    NfaMatcher matcher(start);

    vector<MatchResult> matches = matcher.find_all(text);

    auto end_time = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end_time - start_time);
    total_time_us += duration.count();

    bool success = (matches.size() == (size_t)expected_count);

    if (success) {
      passed_tests++;
      cout << "[✓] ";
    } else {
      failed_tests++;
      cout << "[✗] ";
    }

    cout << setw(50) << left << (test_name.empty() ? pattern : test_name)
         << " | Expected: " << setw(3) << expected_count
         << " | Got: " << setw(3) << matches.size() << " | Time: " << setw(6)
         << duration.count() << " µs";

    if (!success) {
      cout << " FAILED!";
    }
    cout << endl;

  } catch (const exception &e) {
    failed_tests++;
    cout << "[✗] " << setw(50) << left << test_name << " | ERROR: " << e.what()
         << endl;
  }
}

int main() {
  auto program_start = high_resolution_clock::now();

  cout << "===================================================================="
          "\n";
  cout << "           NFA MATCHER - COMPREHENSIVE FIND_ALL TESTS              "
          "\n";
  cout << "===================================================================="
          "\n\n";

  // ========================================================================
  // SECTION 1: BASIC LITERAL MATCHING (10 tests)
  // ========================================================================
  cout << "SECTION 1: Basic Literal Matching (10 tests)\n";
  cout << "----------------------------------------------------------------\n";

  test_find_all_expected("a", "banana", 3, "Single char 'a' in 'banana'");
  test_find_all_expected("an", "banana", 2, "Pattern 'an' in 'banana'");
  test_find_all_expected("na", "banana", 2, "Pattern 'na' in 'banana'");
  test_find_all_expected("cat", "The cat sat on the cat mat", 2,
                         "Word 'cat' twice");
  test_find_all_expected("the", "the quick brown fox jumps over the lazy dog",
                         2, "Word 'the' twice");
  test_find_all_expected("xyz", "xyz abc xyz def xyz", 3,
                         "Pattern 'xyz' three times");
  test_find_all_expected("test", "testing", 1, "Prefix 'test' in 'testing'");
  test_find_all_expected("ing", "testing testing testing", 3,
                         "Suffix 'ing' three times");
  test_find_all_expected("hello", "hello world hello universe hello", 3,
                         "Word 'hello' three times");
  test_find_all_expected("abc", "", 0, "Pattern in empty string");

  // ========================================================================
  // SECTION 2: DOT OPERATOR (10 tests)
  // ========================================================================
  cout << "\nSECTION 2: Dot Operator (.) (10 tests)\n";
  cout << "----------------------------------------------------------------\n";

  test_find_all_expected("c.t", "cat cot cut cxt", 4, "Pattern 'c.t' matches");
  test_find_all_expected("...", "abcdefghijk", 3, "Three-char chunks");
  test_find_all_expected("a.c", "abc adc a c axc", 4, "Pattern 'a.c' matches");
  test_find_all_expected(".", "hello", 5, "Dot matches each char");
  test_find_all_expected("..", "abcd", 2, "Two-char chunks");
  test_find_all_expected("....", "12345678", 2, "Four-char chunks");
  test_find_all_expected("a.b.c", "axbyczdabec", 1,
                         "Pattern 'a.b.c'"); // Fixed: only "axbyc" matches
  test_find_all_expected("t.e", "the tree tie toe", 4,
                         "Pattern 't.e'"); // Fixed: "the", "tre", "tie", "toe"
  test_find_all_expected(".o.", "hello world", 2, "Pattern '.o.'");
  test_find_all_expected("b.t", "bat bet bit bot but bxt", 6,
                         "Pattern 'b.t' six times");

  // ========================================================================
  // SECTION 3: ALTERNATION (10 tests)
  // ========================================================================
  cout << "\nSECTION 3: Alternation (|) (10 tests)\n";
  cout << "----------------------------------------------------------------\n";

  test_find_all_expected("cat|dog", "I have a cat and a dog and another cat", 3,
                         "cat or dog");
  test_find_all_expected("a|e|i|o|u", "hello world", 3,
                         "Vowels in 'hello world'");
  test_find_all_expected("red|blue|green",
                         "red car blue sky green tree red apple", 4, "Colors");
  test_find_all_expected("yes|no", "yes yes no yes no no", 6, "yes or no");
  test_find_all_expected("foo|bar|baz", "foo bar baz foo bar", 5,
                         "Three alternatives");
  test_find_all_expected("abc|def", "abc def abc def abc", 5, "abc or def");
  test_find_all_expected("one|two|three", "one two three one two three", 6,
                         "Numbers");
  test_find_all_expected("cat|dog|bird", "cat bird dog cat bird bird dog", 7,
                         "Animals");
  test_find_all_expected("aa|bb", "aa bb aa bb aa", 5, "aa or bb");
  test_find_all_expected("x|y|z", "x y z x y z x", 7, "x, y, or z");

  // ========================================================================
  // SECTION 4: STAR QUANTIFIER (*) (10 tests)
  // ========================================================================
  cout << "\nSECTION 4: Star Quantifier (*) (10 tests)\n";
  cout << "----------------------------------------------------------------\n";

  test_find_all_expected("a*", "bbb", 4, "Zero or more 'a' in 'bbb'");
  test_find_all_expected(
      "a*", "aaa", 2,
      "Zero or more 'a' in 'aaa'"); // Fixed: greedy matching gives 1 "aaa" + 1
                                    // empty at end
  test_find_all_expected("ba*", "b ba baa baaa", 4, "ba* pattern");
  test_find_all_expected("ab*c", "ac abc abbc abbbc", 4, "ab*c pattern");
  test_find_all_expected("go*d", "gd god good goood", 4, "go*d pattern");
  test_find_all_expected("a*b", "b ab aab aaab", 4, "a*b pattern");
  test_find_all_expected("x*y", "y xy xxy xxxy", 4, "x*y pattern");
  test_find_all_expected("(ab)*", "ab abab ababab", 6, "(ab)* pattern");
  test_find_all_expected("(cat)*", "cat catcat catcatcat", 6, "(cat)* pattern");
  test_find_all_expected(
      "z*", "zzz", 2,
      "z* in 'zzz'"); // Fixed: greedy gives "zzz" + empty at end

  // ========================================================================
  // SECTION 5: PLUS QUANTIFIER (+) (10 tests)
  // ========================================================================
  cout << "\nSECTION 5: Plus Quantifier (+) (10 tests)\n";
  cout << "----------------------------------------------------------------\n";

  test_find_all_expected("a+", "aa aaa aaaa a", 4, "One or more 'a'");
  test_find_all_expected("b+", "b bb bbb bbbb", 4, "One or more 'b'");
  test_find_all_expected("ab+c", "abc abbc abbbc", 3, "ab+c pattern");
  test_find_all_expected("go+d", "god good goood", 3, "go+d pattern");
  test_find_all_expected("a+b+", "ab aab abb aabb", 4, "a+b+ pattern");
  test_find_all_expected("x+y+", "xy xxy xyy xxyy", 4, "x+y+ pattern");
  test_find_all_expected("(ab)+", "ab abab ababab", 3, "(ab)+ pattern");
  test_find_all_expected("(cat)+", "cat catcat catcatcat", 3, "(cat)+ pattern");
  test_find_all_expected("z+", "z zz zzz zzzz", 4, "z+ matches");
  test_find_all_expected("o+", "hello world", 2, "o+ in 'hello world'");

  // ========================================================================
  // SECTION 6: QUESTION QUANTIFIER (?) (10 tests)
  // ========================================================================
  cout << "\nSECTION 6: Question Quantifier (?) (10 tests)\n";
  cout << "----------------------------------------------------------------\n";

  test_find_all_expected("a?b", "b ab aab aaab", 4,
                         "a?b pattern"); // Fixed: matches "b", "ab", "ab", "ab"
  test_find_all_expected("colou?r", "color colour color colour", 4,
                         "Optional 'u'");
  test_find_all_expected("ab?c", "ac abc abbc", 2, "ab?c pattern");
  test_find_all_expected("x?y", "y xy xxy", 3,
                         "x?y pattern"); // Fixed: matches "y", "xy", "y"
  test_find_all_expected("a?a?a?aaa", "aaa aaaa aaaaa aaaaaa", 4,
                         "Multiple optional");
  test_find_all_expected("https?://", "http:// https:// http:// https://", 4,
                         "http(s)?://");
  test_find_all_expected("cats?", "cat cats cat cats", 4, "Optional 's'");
  test_find_all_expected("(ab)?c", "c abc c abc", 4, "Optional group");
  test_find_all_expected("z?", "zzz", 4, "z? in 'zzz'");
  test_find_all_expected("a?", "aaa", 4, "a? in 'aaa'");

  // ========================================================================
  // SECTION 7: RANGE QUANTIFIERS ({m,n}) (10 tests)
  // ========================================================================
  cout << "\nSECTION 7: Range Quantifiers ({m,n}) (10 tests)\n";
  cout << "----------------------------------------------------------------\n";

  test_find_all_expected(
      "a{2}", "a aa aaa aaaa", 4,
      "Exactly 2 'a's"); // Fixed: "aa", "aa" (from aaa), "aa", "aa" (from aaaa)
  test_find_all_expected("a{3}", "a aa aaa aaaa aaaaa", 3, "Exactly 3 'a's");
  test_find_all_expected("a{2,3}", "a aa aaa aaaa aaaaa", 5,
                         "{2,3} quantifier");
  test_find_all_expected("a{1,}", "a aa aaa aaaa", 4, "At least 1 'a'");
  test_find_all_expected("a{2,4}", "a aa aaa aaaa aaaaa aaaaaa", 6,
                         "{2,4} quantifier"); // Fixed
  test_find_all_expected("(ab){2}", "ab abab ababab", 2, "(ab){2} pattern");
  test_find_all_expected("x{3,5}", "xx xxx xxxx xxxxx xxxxxx", 4,
                         "x{3,5} pattern"); // Fixed
  test_find_all_expected("(cat){2,3}", "cat catcat catcatcat catcatcatcat", 3,
                         "(cat){2,3}"); // Fixed
  test_find_all_expected("b{1,2}", "b bb bbb bbbb", 6,
                         "b{1,2} pattern"); // Fixed
  test_find_all_expected("z{0,2}", "z zz zzz", 7,
                         "z{0,2} pattern"); // Fixed: includes empty matches

  // ========================================================================
  // SECTION 8: CHARACTER CLASSES (15 tests)
  // ========================================================================
  cout << "\nSECTION 8: Character Classes (15 tests)\n";
  cout << "----------------------------------------------------------------\n";

  test_find_all_expected("[abc]", "abcdefabc", 6, "Class [abc]");
  test_find_all_expected("[0-9]", "a1b2c3d4", 4, "Digits [0-9]");
  test_find_all_expected("[a-z]", "Hello World", 8, "Lowercase [a-z]");
  test_find_all_expected("[A-Z]", "Hello World", 2, "Uppercase [A-Z]");
  test_find_all_expected("[0-9]+", "Phone: 123-456-7890", 3, "Digit sequences");
  test_find_all_expected("[a-z]+", "Hello World 123", 2, "Word sequences");
  test_find_all_expected("[A-Z][a-z]+", "Hello World Programming", 3,
                         "Capitalized words");
  test_find_all_expected("[aeiou]", "hello", 2, "Vowels in 'hello'");
  test_find_all_expected("[^aeiou]", "hello", 3, "Non-vowels in 'hello'");
  test_find_all_expected("[0-9a-f]+", "abc123def456", 1,
                         "Hex digits"); // Fixed: matches whole "abc123def456"
  test_find_all_expected("[A-Za-z]+", "Hello123World456", 2,
                         "Letter sequences");
  test_find_all_expected("[0-9]{3}", "12 123 1234 12345", 3,
                         "3-digit sequences");
  test_find_all_expected("[a-zA-Z0-9]+", "test123 hello456", 2, "Alphanumeric");
  test_find_all_expected("[^0-9]+", "abc123def456", 2, "Non-digit sequences");
  test_find_all_expected("[xyz]", "xyz abc xyz def", 6, "Class [xyz]");

  // ========================================================================
  // SECTION 9: ANCHORS (^ and $) (10 tests)
  // ========================================================================
  cout << "\nSECTION 9: Anchors (^ and $) (10 tests)\n";
  cout << "----------------------------------------------------------------\n";

  test_find_all_expected("^hello", "hello world hello", 1, "Start anchor ^");
  test_find_all_expected("world$", "hello world hello world", 1,
                         "End anchor $");
  test_find_all_expected("^test$", "test", 1, "Both anchors");
  test_find_all_expected("^abc", "abc def abc", 1, "Start with 'abc'");
  test_find_all_expected("xyz$", "abc xyz abc xyz", 1, "End with 'xyz'");
  test_find_all_expected("^.$", "a", 1, "Single char match");
  test_find_all_expected("^...$", "abc", 1, "Exact 3 chars");
  test_find_all_expected("^[0-9]+$", "12345", 1, "Only digits");
  test_find_all_expected("^test", "test test test", 1,
                         "Multiple but start only");
  test_find_all_expected("end$", "the end the end", 1, "Multiple but end only");

  // ========================================================================
  // SECTION 10: CAPTURE GROUPS (10 tests)
  // ========================================================================
  cout << "\nSECTION 10: Capture Groups (10 tests)\n";
  cout << "----------------------------------------------------------------\n";

  test_find_all_expected("(cat)", "cat dog cat", 2, "Simple group (cat)");
  test_find_all_expected("(a+)(b+)", "aabbb aaab ab", 3, "Two groups (a+)(b+)");
  test_find_all_expected("(cat|dog)", "cat dog cat dog", 4,
                         "Group with alternation");
  test_find_all_expected("([0-9]+)-([0-9]+)", "123-456 789-012", 2,
                         "Hyphenated numbers");
  test_find_all_expected("((a)(b))", "ab ab ab", 3, "Nested groups");
  test_find_all_expected("(x+)(y+)(z+)", "xyz xxyyyzz", 2, "Three groups");
  test_find_all_expected("(foo)+(bar)+", "foobar foofoobarbar", 2,
                         "Repeated groups");
  test_find_all_expected("(test)", "test test test", 3,
                         "Simple repeated group");
  test_find_all_expected("(a)(b)(c)", "abc abc", 2, "Three single-char groups");
  test_find_all_expected("(hello)|(world)", "hello world hello world", 4,
                         "Alternative groups");

  // ========================================================================
  // SECTION 11: COMPLEX PATTERNS (10 tests)
  // ========================================================================
  cout << "\nSECTION 11: Complex Patterns (10 tests)\n";
  cout << "----------------------------------------------------------------\n";

  test_find_all_expected("(a|b)+", "aaa bbb aaabbb", 3, "Alternation with +");
  test_find_all_expected(
      "(a|b)*c", "c ac bc aac bbc aaabbbcc", 7,
      "Alternation with * then c"); // Fixed: includes empty matches before each
                                    // 'c'
  test_find_all_expected("[0-9]{3}-[0-9]{4}", "Call 555-1234 or 555-5678", 2,
                         "Phone format");
  test_find_all_expected("\\([0-9]+\\)", "Numbers: (123) and (456) here", 2,
                         "Parenthesized nums");
  test_find_all_expected("[a-z]+@[a-z]+", "john@example and jane@test", 2,
                         "Simple email pattern");
  test_find_all_expected("(ab|cd)+", "ab cd abcd ababcd cdcdab", 5,
                         "Complex alternation");
  test_find_all_expected("[A-Z][a-z]*", "The Quick Brown Fox", 4,
                         "Capitalized words");
  test_find_all_expected("(\\.|!|\\?)", "Hi. How are you? I'm fine!", 3,
                         "Punctuation");
  test_find_all_expected(
      "a.*b", "axb ayb axyb", 1,
      "Greedy .* between a and b"); // Fixed: greedy consumes "axb ayb axyb"
  test_find_all_expected("(foo|bar)\\.", "foo. bar. baz.", 2, "Word then dot");

  // ========================================================================
  // SECTION 12: EDGE CASES (10 tests)
  // ========================================================================
  cout << "\nSECTION 12: Edge Cases (10 tests)\n";
  cout << "----------------------------------------------------------------\n";

  test_find_all_expected("", "abc", 4, "Empty pattern");
  test_find_all_expected("a", "", 0, "Pattern on empty text");
  test_find_all_expected("", "", 1, "Both empty");
  test_find_all_expected("aa", "aaaa", 2, "Non-overlapping 'aa'");
  test_find_all_expected("aba", "abababa", 2, "Non-overlapping 'aba'");
  test_find_all_expected("aaa", "aaaaaa", 2, "Non-overlapping 'aaa'");
  test_find_all_expected("abc", "abc", 1, "Exact match");
  test_find_all_expected(".*", "test", 2, ".* matches");
  // Removed test for a+? as it causes syntax error (lazy quantifiers not
  // supported)
  test_find_all_expected("(a*)*", "aaa", 2, "Nested stars"); // Fixed

  // ========================================================================
  // SECTION 13: REAL-WORLD PATTERNS (15 tests)
  // ========================================================================
  cout << "\nSECTION 13: Real-World Patterns (15 tests)\n";
  cout << "----------------------------------------------------------------\n";

  test_find_all_expected("[a-zA-Z0-9]+@[a-zA-Z]+\\.[a-zA-Z]+",
                         "Contact: john@example.com or jane@test.org", 2,
                         "Email addresses");
  test_find_all_expected("https://[a-z]+\\.[a-z]+",
                         "Visit https://google.com and https://github.com", 2,
                         "HTTPS URLs");
  test_find_all_expected("[A-Z][a-z]+",
                         "The Quick Brown Fox Jumps Over The Lazy Dog", 9,
                         "Capitalized words");
  test_find_all_expected("[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}",
                         "IPs: 192.168.1.1 and 10.0.0.1", 2, "IP addresses");
  test_find_all_expected("[A-Z]{2,}", "USA UK NATO FBI CIA", 5, "Acronyms");
  test_find_all_expected("#[0-9a-fA-F]{6}", "Colors: #FF5733 #C70039 #900C3F",
                         3, "Hex colors");
  test_find_all_expected("\\$[0-9]+\\.[0-9]{2}", "Prices: $19.99 and $29.99", 2,
                         "Prices");
  test_find_all_expected("[0-9]{4}-[0-9]{2}-[0-9]{2}",
                         "Dates: 2024-01-15 and 2024-12-31", 2, "ISO dates");
  test_find_all_expected("v[0-9]+\\.[0-9]+\\.[0-9]+",
                         "Versions: v1.0.0 v2.1.3 v10.5.2", 3,
                         "Version numbers");
  test_find_all_expected("[0-9]+%", "Growth: 25% 50% 75% 100%", 4,
                         "Percentages");
  test_find_all_expected("@[a-zA-Z0-9_]+", "Follow @user123 and @john_doe", 2,
                         "Twitter handles");
  test_find_all_expected("\\+[0-9]{1,3}-[0-9]{3}-[0-9]{4}",
                         "Call +1-555-1234 or +44-207-1234", 2,
                         "Phone numbers");
  test_find_all_expected("[A-Z]{2}-[0-9]{4}", "Flight AA-1234 and BA-5678", 2,
                         "Flight codes"); // Fixed: 2 letters not 3
  test_find_all_expected("\\([0-9]{3}\\) [0-9]{3}-[0-9]{4}",
                         "Tel: (555) 123-4567 or (555) 987-6543", 2,
                         "US phone format");
  test_find_all_expected("[0-9]{5}(-[0-9]{4})?", "ZIP: 12345 or 12345-6789", 2,
                         "ZIP codes");

  // ========================================================================
  // PERFORMANCE STRESS TESTS (5 tests with larger inputs)
  // ========================================================================
  cout << "\nSECTION 14: Performance Tests (5 tests)\n";
  cout << "----------------------------------------------------------------\n";

  // Generate large text for stress testing
  string large_text_1(10000, 'a');
  test_find_all_expected("a", large_text_1, 10000, "10,000 'a' chars");

  string large_text_2;
  for (int i = 0; i < 1000; i++)
    large_text_2 += "abc";
  test_find_all_expected("abc", large_text_2, 1000, "1,000 'abc' patterns");

  string large_text_3;
  for (int i = 0; i < 500; i++)
    large_text_3 += "test123 ";
  test_find_all_expected("[0-9]+", large_text_3, 500, "500 number sequences");

  string large_text_4;
  for (int i = 0; i < 200; i++)
    large_text_4 += "hello world ";
  test_find_all_expected("hello|world", large_text_4, 400, "400 alternations");

  string large_text_5;
  for (int i = 0; i < 100; i++)
    large_text_5 += "john@example.com ";
  test_find_all_expected("[a-z]+@[a-z]+\\.[a-z]+", large_text_5, 100,
                         "100 emails");

  // ========================================================================
  // FINAL STATISTICS
  // ========================================================================
  auto program_end = high_resolution_clock::now();
  auto total_duration =
      duration_cast<milliseconds>(program_end - program_start);

  cout << "\n=================================================================="
          "==\n";
  cout << "                        TEST SUMMARY                                "
          "\n";
  cout << "===================================================================="
          "\n";
  cout << "Total Tests:      " << total_tests << endl;
  cout << "Passed:           " << passed_tests << " (" << fixed
       << setprecision(1) << (100.0 * passed_tests / total_tests) << "%)"
       << endl;
  cout << "Failed:           " << failed_tests << " (" << fixed
       << setprecision(1) << (100.0 * failed_tests / total_tests) << "%)"
       << endl;
  cout << "--------------------------------------------------------------------"
          "\n";
  cout << "Total Execution Time:     " << total_duration.count() << " ms"
       << endl;
  cout << "Average Time per Test:    " << (total_time_us / total_tests) << " µs"
       << endl;
  cout << "===================================================================="
          "\n";

  if (failed_tests == 0) {
    cout << "\n ALL TESTS PASSED!\n";
  } else {
    cout << "\n  SOME TESTS FAILED \n";
  }
  cout << "\n";

  return failed_tests > 0 ? 1 : 0;
}