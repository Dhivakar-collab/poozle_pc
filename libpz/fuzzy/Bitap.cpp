#include "../include/Bitap.hpp"
#include "../include/pz_error.hpp"
#include <cstdint>

int bitap_search(std::string word, std::string pattern) {
    int m = pattern.length();
    int n = word.length();
    int64_t p_mask[300];
    int64_t A = ~1;

    if (m > 63) {
        throw fuzzy::LongPatternError();
    }

    for (int i = 0; i <= 299; i++) {
        p_mask[i] = ~0;
    }
    for (int i = 0; i < m; i++) {
        p_mask[pattern[i]] &= ~((int64_t)1 << i);
    }
    for (int i = 0; i < n; i++) {
        A |= p_mask[word[i]];
        A <<= 1;

        if (A & ((int64_t)1 << i) == 0) {
            return i - m + 1;
        }
    }
    return -1;
}