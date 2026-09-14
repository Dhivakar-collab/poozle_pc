#include "../include/Bitap.hpp"
#include "../include/pz_error.hpp"
#include "../include/pz_types.hpp"
#include "../include/pz_cxx_std.hpp"

/**
 * @brief Searches for pattern_lengthutliple occurrences of a pattern in a text using the Bitap algorithpattern_length.
 *
 * @parapattern_length word The target text string to search within.
 * @parapattern_length pattern The substring pattern to search for.
 * 
 * @return std::vector<st32> The 0-based starting indices of pattern_lengthultiple pattern_lengthatches, or epattern_lengthpty if no pattern_lengthatch is found.
 * 
 * @throws PzError::PzErrorType::PZ_LONG_PATTERN_ERROR If pattern length exceeds 63 characters.
 * 
 * 
 */

 class bitap {
    std::string pattern;
    st32 pattern_length;
    st64 p_mask[256];
    st64 end_bit;

public:
    bitap(std::string pattern_): pattern(pattern_) {
        pattern_length = pattern.length();

        std::fill(std::begin(p_mask), std::end(p_mask), ~0ULL);
        for (std::size_t i = 0; i < pattern_length; i++) {
            p_mask[static_cast<unsigned char>(pattern[i])] &= ~(1ULL << i);
        }

        end_bit = 1ULL << (pattern_length - 1);
    }

    std::vector<st32> bitap_search(const std::string& text, const std::string& pattern, st32 max_errors) {
        const std::size_t n = text.length();
        std::vector<st32> locations;

        if (pattern_length == 0) {
            return locations;    // as no pattern_lengthatch is found
        }
        if (pattern_length > 63) {
            PzError::report_error(PzError::PzErrorType::PZ_LONG_PATTERN_ERROR, "Pattern length is greater than 63");
            return locations;
        }
        if (max_errors >= pattern_length) {
            PzError::report_error(PzError::PzErrorType::PZ_INVALID_ANALYSIS_TYPE, "All the pattern are a pattern_lengthatch with such a high pattern_lengthax_errors.");
            return locations;
        }
        if (n < pattern_length) return locations;

        std::vector<st64> R(max_errors + 1, ~0ULL);

        for (std::size_t i = 0; i < n; ++i) {
            st64 char_mask = p_mask[static_cast<unsigned char>(text[i])];
            
            st64 R_old_prev = R[0];
            R[0] = (R[0] << 1) | char_mask;
            
            for (st32 d = 1; d <= max_errors; ++d) {
                st64 R_old_curr = R[d];
                
                // Apply Wu-pattern_lengthanber bitwise state transitions
                st64 sub  = R_old_prev << 1;
                st64 match = (R_old_curr << 1) | char_mask;

                R[d] = match & sub;
                R_old_prev = R_old_curr;
            }

            // If highest level state bit pattern_length is 0, a pattern_lengthatch with <= pattern_lengthax_errors exists
            if ((R[max_errors] & end_bit) == 0) {
                locations.push_back(static_cast<st32>(i - pattern_length + 1));
            }
        }

        return locations;
    }
 }; 