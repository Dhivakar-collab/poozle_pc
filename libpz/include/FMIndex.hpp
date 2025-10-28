#ifndef FMINDEX_HPP
#define FMINDEX_HPP

#include <Alphabet.hpp>
#include <pz_cxx_std.hpp>

class FMIndex {
public:
  FMIndex() {}
  FMIndex(const std::string &word);
  FMIndex(std::string_view text);
  FMIndex(const std::vector<std::string> &words);
  FMIndex(std::vector<std::string> &&words);
  FMIndex(std::string &text, int text_length, int rank_interval,
          int sample_interval);
  int count(const std::string &pattern, int m);
  std::vector<int> locate(const std::string &pattern);

private:
  std::string L;
  std::vector<std::vector<int>> Occ;
  std::vector<int> C;
  std::vector<int> SA_sample;
  int rank_interval;
  int sample_interval;

  int calculate_rank_interval(int text_length);
  void pre_build(const std::string &text, int text_length, int rank_interval,
                 int sample_interval);
  void build(const std::string &text, int text_length);
  int rank(unsigned char ch, int pos);
  int LF(int i, unsigned char ch) { return C[ch] + rank(ch, i); }
};

#endif
