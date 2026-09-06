// Ukkonen Cutoff DP
#include "ukkonen.hpp"
#include "pz_cxx_std.hpp"

bool Ukkonen::edit_within_k(const std::string& s, const std::string& t, unsigned int k) {
    int m = s.size();
    int n = t.size();
    int limit= static_cast<int>(k);
    if(std::abs(m-n) > k) return false;
    int INF= limit + 1;
    std::vector<int> prev(2*limit + 3, INF);
    std::vector<int> curr(2*limit + 3, INF);

    //Converting an empty pattern into a j-character string requires j insertions.
    //Base case
    for(int j = 0; j <= std::min(n, limit); j++) {
        int d=j;
        prev[d+k+1] = j;
    }

    for(int i= 1; i <= m; i++) {
        curr[0] = i; //Converting an i-character string into an empty pattern requires i deletions.
        //We only need to consider the diagonals from i-k to i+k

        int left = std::max(1, i-limit);
        int right = std::min(n, i+limit);

        for (int j=left; j<= right; j++) {
            int d = j-i;
            int idx = d+k+1;
            // The index at which we store the edit distance for converting
            // substring s[0..i-1] to substring t[0..j-1] is d+k+1, where d = j-i.

            int cost = (s[i-1] == t[j-1]) ? 0 : 1;
            curr[idx] = std::min({
                prev[idx+1] + 1, // (j-(i-1) + k + 1) = idx+1, represents deletion
                curr[idx-1] + 1, // insertion
                prev[idx] + cost // replacement
            });
        }
        swap(prev, curr);
    }
    return prev[n-m+limit+1] <= limit;
}

std::vector<std::pair<int, int>> Ukkonen::locate(const std::string &pattern,
                                                const std::string &text,
                                                unsigned int k) {
    std::vector<std::pair<int, int>> locations;
    
    //We do not consider empty substrings or empty patterns as matches.
    if(pattern.empty() || text.empty()) {
        return locations;
    }
    int m = pattern.size();
    int n = text.size();
    int limit= static_cast<int>(k);

    //A substring whose length differs from the pattern by more than k cannot be a match.
    int min_length = std::max(1, m - limit);
    int max_length = m + limit;

    //Values larger than k are irrelevant as we only want edit-distance <= k.
    int INF = limit + 1;

    //Try every possible starting position in the text.
    for(int start=0; start<n; start++){
        int remaining = n-start;

        if(remaining < min_length) break; //String left is too short

        int max_j = std::min(max_length, remaining);
        std::vector<int> prev(2*limit + 3, INF);
        std::vector<int> curr(2*limit + 3, INF);

        //Converting an empty pattern into a j-character string requires j insertions.
        //Base case
        for(int j = 0; j <= std::min(max_j, limit); j++) {
            int d=j;
            prev[d+limit+1] = j;
        }

        for(int i= 1; i <= m; i++) {
            int d_zero = -i;
            if (d_zero >= -limit  && d_zero <= limit ) {
                curr[d_zero + limit + 1] = i;
            } 
            //Converting an i-character string into an empty pattern requires i deletions.
            //We only need to consider the diagonals from i-limit to i+limit

            int left = std::max(1, i-limit);
            int right = std::min(max_j, i+limit);

            for (int j=left; j<= right; j++) {
                int d = j-i;
                int idx = d+limit+1;
                // The index at which we store the edit distance for converting
                // substring s[0..i-1] to substring t[0..j-1] is d+limit+1, where d = j-i.

                int cost = (pattern[i-1] == text[start+ j-1]) ? 0 : 1;
                curr[idx] = std::min({
                    prev[idx+1] + 1, // (j-(i-1) + k + 1) = idx+1, represents deletion
                    curr[idx-1] + 1, // insertion
                    prev[idx] + cost // match/substitution
                });
            }
            swap(prev, curr);
        }
        //The final DP row contains the edit distance between the complete pattern
        //and every relevant substring length.
        
        int left = std::max(1, m - limit);

        for (int j = left; j <= max_j; j++) {

            int d = j - m;
            int idx = d + limit + 1;

            // Every qualifying substring is recorded separately.
            if (prev[idx] <= limit) {
                int end = start + j - 1;
                locations.emplace_back(start, end);
            }
        }
    }
    return locations;
}

int Ukkonen::count(const std::string& pattern,
                   const std::string& text,
                   unsigned int k) {
    // locate() defines the exact set of fuzzy matches.
    // Therefore, the number of matches is simply the number
    // of elements returned by locate().
    return static_cast<int>(locate(pattern, text, k).size());
}