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

 class LayeredFuzzyMatcher {
public:
    struct Match {
        st32 start_index;
        st32 end_index;
        st32 distance;
    };

    explicit LayeredFuzzyMatcher(const std::string& pattern)
        : pattern_(pattern), m_(static_cast<st32>(pattern.length())) {
        if (m_ > 64) {
            PzError::report_error(PzError::PzErrorType::PZ_LONG_PATTERN_ERROR,
                                  "Pattern length is greater than 64");
        }
    }

    std::vector<Match> search(const std::string& text, st32 max_errors) const {
        std::vector<Match> results;
        if (m_ == 0 || text.empty()) return results;

        if (max_errors < 0 || max_errors >= m_) {
            PzError::report_error(PzError::PzErrorType::PZ_INVALID_ANALYSIS_TYPE,
                                  "max_errors must satisfy 0 <= max_errors < pattern length");
        }

        const st32 n = static_cast<st32>(text.length());
        if (n < m_ - max_errors) return results;

        // Stage 1: Bitap Filter with initial deletion support
        std::vector<st32> candidate_ends = run_bitap_filter(text, max_errors);

        // Stage 2: Local Bounded DP Verification
        for (st32 end_idx : candidate_ends) {
            Match m;
            if (verify_local_dp(text, end_idx, max_errors, m)) {
                results.push_back(m);
            }
        }

        return results;
    }

private:
    std::string pattern_;
    st32 m_;

    std::vector<st32> run_bitap_filter(const std::string& text, st32 max_errors) const {
        std::vector<st32> candidates;
        const st32 n = static_cast<st32>(text.length());

        const st64 full_mask = (m_ == 64) ? ~0ULL : ((1ULL << m_) - 1);
        const st64 end_bit = 1ULL << (m_ - 1);

        st64 p_mask[256];
        std::fill(std::begin(p_mask), std::end(p_mask), full_mask);
        for (st32 i = 0; i < m_; ++i) {
            p_mask[static_cast<unsigned char>(pattern_[i])] &= ~(1ULL << i);
        }

        std::vector<st64> R(max_errors + 1, full_mask);

        for (st32 i = 0; i < n; ++i) {
            // Seed free-start matching with initial deletions at position i
            R[0] &= ~1ULL; // Bit 0 active (exact start)
            for (st32 d = 1; d <= max_errors; ++d) {
                R[d] &= (R[d - 1] << 1) | ~full_mask; // Bit d active via d initial deletions
            }

            st64 char_mask = p_mask[static_cast<unsigned char>(text[i])];
            std::vector<st64> newR(max_errors + 1, full_mask);

            newR[0] = ((R[0] << 1) | char_mask) & full_mask;

            for (st32 d = 1; d <= max_errors; ++d) {
                st64 match = (R[d] << 1) | char_mask;
                st64 sub   = R[d - 1] << 1;
                st64 del   = R[d - 1];
                st64 ins   = newR[d - 1] << 1;

                newR[d] = (match & sub & del & ins) & full_mask;
            }

            R = std::move(newR);

            if ((R[max_errors] & end_bit) == 0) {
                candidates.push_back(i);
            }
        }
        return candidates;
    }

    bool verify_local_dp(const std::string& text, st32 end_idx, st32 max_errors, Match& out_match) const {
        st32 w_start = std::max(0, end_idx - m_ - max_errors + 1);
        st32 w_len = end_idx - w_start + 1;

        std::string W = text.substr(w_start, w_len);

        const st32 INF = m_ + w_len + 1;
        std::vector<std::vector<st32>> dp(w_len + 1, std::vector<st32>(m_ + 1, INF));
        std::vector<std::vector<st32>> start(w_len + 1, std::vector<st32>(m_ + 1, 0));

        for (st32 j = 0; j <= m_; ++j) { dp[0][j] = j; start[0][j] = w_start; }
        for (st32 i = 0; i <= w_len; ++i) { dp[i][0] = 0; start[i][0] = w_start + i; }

        for (st32 i = 1; i <= w_len; ++i) {
            for (st32 j = 1; j <= m_; ++j) {
                st32 cost = (pattern_[j - 1] == W[i - 1]) ? 0 : 1;

                st32 best_val = dp[i - 1][j - 1] + cost;
                st32 best_start = start[i - 1][j - 1];

                st32 del_val = dp[i - 1][j] + 1;
                if (del_val < best_val) { best_val = del_val; best_start = start[i - 1][j]; }

                st32 ins_val = dp[i][j - 1] + 1;
                if (ins_val < best_val) { best_val = ins_val; best_start = start[i][j - 1]; }

                dp[i][j] = best_val;
                start[i][j] = best_start;
            }
        }

        st32 final_dist = dp[w_len][m_];
        if (final_dist <= max_errors) {
            out_match.start_index = start[w_len][m_];
            out_match.end_index = end_idx;
            out_match.distance = final_dist;
            return true;
        }

        return false;
    }
};