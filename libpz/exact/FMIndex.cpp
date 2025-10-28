#include "FMIndex.hpp"

static std::vector<int> build_suffix_array(const std::string &input) {
  std::string text = input;
  int len = (int)text.size();
  std::vector<int> order(len), cls(len);
  std::vector<std::pair<char, int>> a(len);

  for (int i = 0; i < len; ++i)
    a[i] = {text[i], i};
  std::sort(a.begin(), a.end());
  for (int i = 0; i < len; ++i)
    order[i] = a[i].second;
  cls[order[0]] = 0;
  for (int i = 1; i < len; ++i)
    cls[order[i]] = cls[order[i - 1]] + (a[i].first != a[i - 1].first);

  int k = 0;
  while ((1 << k) < len) {
    for (int i = 0; i < len; ++i)
      order[i] = (order[i] - (1 << k) + len) % len;

    int n = len;
    std::vector<int> cnt(n, 0), pos(n, 0), new_order(n);
    for (int x : cls)
      cnt[x]++;
    pos[0] = 0;
    for (int i = 1; i < n; ++i)
      pos[i] = pos[i - 1] + cnt[i - 1];

    for (int idx = 0; idx < n; ++idx) {
      int v = order[idx];
      int c = cls[v];
      new_order[pos[c]] = v;
      pos[c]++;
    }
    order.swap(new_order);

    std::vector<int> new_cls(n);
    new_cls[order[0]] = 0;
    for (int i = 1; i < n; ++i) {
      std::pair<int, int> prev = {cls[order[i - 1]],
                                  cls[(order[i - 1] + (1 << k)) % n]};
      std::pair<int, int> now = {cls[order[i]], cls[(order[i] + (1 << k)) % n]};
      new_cls[order[i]] = new_cls[order[i - 1]] + (prev != now);
    }
    cls.swap(new_cls);
    k++;
  }
  return order;
}

int FMIndex::calculate_rank_interval(int text_length) {
  int num = 32 * ALPHABET_SIZE;
  int den = 1000000000 - (40 * ALPHABET_SIZE);
  if (den <= 0) {
    return 80;
  }
  int res = num / den;
  if (res == 0) {
    return 1;
  }
  return res;
}

FMIndex::FMIndex(const std::string &word) {
  std::string text;
  for (auto c : word) {
    text = text + get_position(c);
  }
  text = text + RESERVED_SYMBOLS["EOB"];
  pre_build(text, (int)text.size(), calculate_rank_interval(text.size()), 1);
}

FMIndex::FMIndex(std::string_view text_view) {
  std::string text;
  for (auto c : text_view) {
    text = text + get_position(c);
  }
  text = text + RESERVED_SYMBOLS["EOB"];
  pre_build(text, (int)text.size(), calculate_rank_interval(text.size()), 1);
}

FMIndex::FMIndex(const std::vector<std::string> &words) {
  std::string text;
  for (auto word : words) {
    for (auto c : word) {
      text = text + get_position(c);
    }
    text = text + RESERVED_SYMBOLS["CONCATNATION"];
  }
  text = text + RESERVED_SYMBOLS["EOB"];
  pre_build(text, (int)text.size(), calculate_rank_interval(text.size()), 1);
}

FMIndex::FMIndex(std::vector<std::string> &&words) {
  std::string text;
  for (auto word : words) {
    for (auto c : word) {
      text = text + get_position(c);
    }
    text = text + RESERVED_SYMBOLS["CONCATNATION"];
  }
  text = text + RESERVED_SYMBOLS["EOB"];
  pre_build(text, (int)text.size(), calculate_rank_interval(text.size()), 1);
}

void FMIndex::pre_build(const std::string &text, int text_length,
                        int rank_interval, int sample_interval) {
  this->rank_interval = rank_interval;
  this->sample_interval = std::max(1, sample_interval);
  int n = text_length;
  std::vector<int> sa = build_suffix_array(text);

  L.assign(n, 0);
  SA_sample.assign(n, -1);

  int sa_len = (int)sa.size();
  int out_i = 0;

  for (int i = 0; i < sa_len; ++i) {
    int p = sa[i];

    if (p == n) {
      continue;
    }

    int prev = (p - 1 + sa_len) % sa_len;

    if (prev == n) {
      L[out_i] = text[n - 1];
    } else {
      L[out_i] = text[prev];
    }

    if (p < n && (p % this->sample_interval == 0)) {
      SA_sample[out_i] = p;
    }

    ++out_i;
  }

  int occ_rows = n / rank_interval + 1;
  Occ.assign(occ_rows, std::vector<int>(ALPHABET_SIZE, 0));
  C.assign(ALPHABET_SIZE + 1, 0);
  build(text, text_length);
}

FMIndex::FMIndex(std::string &text, int text_length, int rank_interval,
                 int sample_interval) {
  pre_build(text, text_length, rank_interval, sample_interval);
  build(text, text_length);
}

void FMIndex::build(const std::string &text, int text_length) {
  int n = text_length;

  std::vector<int> freq(ALPHABET_SIZE, 0);
  for (int i = 0; i < n; ++i) {
    unsigned char uc = static_cast<unsigned char>(text[i]);
    if (uc < ALPHABET_SIZE)
      freq[uc]++;
  }
  C[0] = 0;
  for (int c = 1; c <= ALPHABET_SIZE; ++c)
    C[c] = C[c - 1] + ((c - 1 < ALPHABET_SIZE) ? freq[c - 1] : 0);

  std::vector<int> tmp(ALPHABET_SIZE, 0);
  for (int i = 1; i <= n; ++i) {
    unsigned char uc = static_cast<unsigned char>(L[i - 1]);
    if (uc < ALPHABET_SIZE)
      tmp[uc]++;

    if (i % rank_interval == 0) {
      int idx = i / rank_interval;
      if (idx >= Occ.size())
        continue;
      for (int j = 0; j < ALPHABET_SIZE; ++j) {
        Occ[idx][j] = Occ[idx - 1][j] + tmp[j];
        tmp[j] = 0;
      }
    }
  }
}

int FMIndex::rank(unsigned char ch, int pos) {
  if (pos <= 0)
    return 0;
  if (pos > L.length())
    pos = L.length();

  int idx = pos / rank_interval;
  if (idx >= (int)Occ.size())
    idx = (int)Occ.size() - 1;

  int count = Occ[idx][ch];
  int start = idx * rank_interval;

  for (int i = start; i < pos; ++i)
    if (static_cast<unsigned char>(L[i]) == ch)
      ++count;

  return count;
}

int FMIndex::count(const std::string &pattern, int m) {
  if (m == 0)
    return 0;

  unsigned char ch = get_position(static_cast<unsigned char>(pattern[m - 1]));
  if (ch >= ALPHABET_SIZE)
    return 0;
  int s = C[ch];
  int e = C[ch + 1] - 1;

  int i = m - 2;
  while (s <= e && i >= 0) {
    ch = get_position(static_cast<unsigned char>(pattern[i]));
    if (ch >= ALPHABET_SIZE)
      return 0;

    int new_s = C[ch] + rank(ch, s);
    int new_e = C[ch] + rank(ch, e + 1) - 1;
    s = new_s;
    e = new_e;
    --i;
  }

  return (s <= e) ? (e - s + 1) : 0;
}

std::vector<int> FMIndex::locate(const std::string &pattern) {
  int m = (int)pattern.size();
  std::vector<int> results;
  if (m == 0)
    return results;

  unsigned char ch = get_position(static_cast<unsigned char>(pattern[m - 1]));
  if (ch >= ALPHABET_SIZE)
    return results;
  int s = C[ch];
  int e = C[ch + 1] - 1;

  for (int i = m - 2; i >= 0 && s <= e; --i) {
    ch = get_position(static_cast<unsigned char>(pattern[i]));
    if (ch >= ALPHABET_SIZE)
      return results;
    s = C[ch] + rank(ch, s);
    e = C[ch] + rank(ch, e + 1) - 1;
  }

  if (s > e)
    return results;

  for (int row = s; row <= e; ++row) {
    int pos = row;
    int steps = 0;

    while (SA_sample[pos] == -1) {
      unsigned char c = get_position(static_cast<unsigned char>(L[pos]));
      if (c >= ALPHABET_SIZE)
        break;
      pos = LF(pos, c);
      ++steps;

      if (steps > L.length())
        break;
    }

    if (SA_sample[pos] != -1) {
      int start = (SA_sample[pos] + steps) % L.length();
      results.push_back(start);
    }
  }

  std::sort(results.begin(), results.end());
  return results;
}
