#ifndef NFA_MATCHER_HPP
#define NFA_MATCHER_HPP
#include "nfa.hpp"
#include <set>
#include <utility>

// Represents the result of a matching operation
struct MatchResult {
  bool matched = false;
  int start_pos = -1;
  int end_pos = -1;
  std::vector<std::pair<int, int>>
      captures; // Group captures: (start, end) pairs
};

class NfaMatcher {
public:
  explicit NfaMatcher(State *start_state);

  // Match from the beginning of the string (prefix match)
  MatchResult match(const std::string_view &text);

  // Find all non-overlapping matches in the text (similar to Python's
  // re.findall)
  std::vector<MatchResult> find_all(const std::string_view &text);

  // This function adds a backslash before each regex metacharacter,
  // so they are treated as literal characters in a pattern.
  std::string escape(const std::string_view &text);

private:
  State *start;
  int generation = 0; // Used to avoid revisiting states in the same step

  // Core matching function
  MatchResult match_internal(const std::string_view &text, int start_pos,
                             bool anchored_start, bool anchored_end);

  // Follow epsilon transitions from a single state
  void follow_epsilons(State *state, std::set<State *> &result_set,
                       std::vector<int> &captures, bool at_start, bool at_end);

  // Get all states reachable via epsilon transitions from a set of states
  std::set<State *> follow_epsilons_from_set(const std::set<State *> &states,
                                             std::vector<int> &captures,
                                             bool at_start, bool at_end);

  // Check if a character matches a state's condition
  bool state_matches(State *state, char c);

  // Get all matching transitions from current states with a character
  std::set<State *> get_next_states(const std::set<State *> &current_states,
                                    char c, std::vector<int> &captures,
                                    bool at_start, bool at_end);
};

#endif // NFA_MATCHER_HPP