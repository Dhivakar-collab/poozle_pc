#include "../include/Bitap.hpp"
#include "../include/pz_error.hpp"
#include "../include/pz_types.hpp"
#include <cstdint>
#include <vector>

/**
 * @brief Searches for mutliple occurrences of a pattern in a text using the Bitap algorithm.
 *
 * @param word The target text string to search within.
 * @param pattern The substring pattern to search for.
 * 
 * @return std::vector<st32> The 0-based starting indices of multiple matches, or empty if no match is found.
 * 
 * @throws PzError::PzErrorType::PZ_LONG_PATTERN_ERROR If pattern length exceeds 63 characters.
 */
std::vector<st32> bitap_search(std::string word, std::string pattern) {
    const std::size_t m = pattern.length();
    const std::size_t n = word.length();

    std::vector<st32> locations; 

    if (m == 0) {
        return locations;
    }
    if (m > 63) {
        PzError::report_error(PzError::PzErrorType::PZ_LONG_PATTERN_ERROR, "Pattern length is greater than 63");
        return locations;
    }

    /** Precomputed bitmask for each character byte (0-255). */
    st64 p_mask[256];
    
    /** State bitmask. Bit 0 is initialized to 0 to accept new match starts. */
    st64 A = ~1ULL;

    // Initialize all character masks to all 1-bits (no matches)
    for (st32 i = 0; i < 256; ++i) {
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
            locations.push_back(static_cast<int>(i - m + 1));
        }
    }

    return locations;
}