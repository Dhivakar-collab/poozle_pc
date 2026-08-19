#include "../include/Bitap.hpp"
#include "../include/pz_error.hpp"
#include <cstdint>

/**
 * @brief Searches for the first occurrence of a pattern in a text using the Bitap algorithm.
 *
 * @param word The target text string to search within.
 * @param pattern The substring pattern to search for.
 * 
 * @return int The 0-based starting index of the first match, or -1 if no match is found.
 * 
 * @throws fuzzy::LongPatternError If pattern length exceeds 63 characters.
 */
int bitap_search(std::string word, std::string pattern) {
    const std::size_t m = pattern.length();
    const std::size_t n = word.length();

    if (m == 0) {
        return 0;
    }
    if (m > 63) {
        throw fuzzy::LongPatternError();
    }

    /** Precomputed bitmask for each character byte (0-255). */
    uint64_t p_mask[256];
    
    /** State bitmask. Bit 0 is initialized to 0 to accept new match starts. */
    uint64_t A = ~1ULL;

    // Initialize all character masks to all 1-bits (no matches)
    for (int i = 0; i < 256; ++i) {
        p_mask[i] = ~0ULL;
    }

    // Set bit 'i' to 0 wherever character pattern[i] appears
    for (std::size_t i = 0; i < m; ++i) {
        p_mask[static_cast<unsigned char>(pattern[i])] &= ~(1ULL << i);
    }

    // Process each character of the text
    for (std::size_t i = 0; i < n; ++i) {
        A |= p_mask[static_cast<unsigned char>(word[i])];
        A <<= 1;

        // Check if bit 'm' is 0 (indicating a full pattern match ended at index i)
        if ((A & (1ULL << m)) == 0) {
            return static_cast<int>(i - m + 1);
        }
    }

    return -1;
}