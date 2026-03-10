#ifndef NFA_MATCHER_HPP
#define NFA_MATCHER_HPP
#include "Nfa.hpp"
#include "pz_cxx_std.hpp"
#include "pz_types.hpp"

// Represents the result of a matching operation
struct MatchResult {
  bool matched = false;
  st32 start_pos = -1;
  st32 end_pos = -1;
  std::vector<std::pair<st32, st32>>
      captures; // Group captures: (start, end) pairs

  // constructor
  MatchResult(bool matched_, st32 start_pos_, st32 end_pos_,
              std::vector<std::pair<st32, st32>> captures_)
      : matched(matched_), start_pos(start_pos_), end_pos(end_pos_),
        captures(captures_) {}

  // move constructor
  MatchResult(MatchResult &&rhs) noexcept
      : matched(std::move(rhs.matched)), start_pos(std::move(rhs.start_pos)),
        end_pos(std::move(rhs.end_pos)), captures(std::move(captures)) {}
};

class NfaMatcher {
public:
  explicit NfaMatcher(State *start_state);

  // Match from the beginning of the string (prefix match)
  MatchResult match(const std::string_view &text);

  // Find all non-overlapping matches in the text (similar to Python's
  // re.findall)
  std::vector<MatchResult> find_all(const std::string_view &text);

  // This function adds a backslash before each regex metaut8acter,
  // so they are treated as literal ut8acters in a pattern.
  std::string escape(const std::string_view &text);

private:
  State *start;
  st32 generation = 0; // Used to avoid revisiting states in the same step

  // Core matching function
  MatchResult match_internal(const std::string_view &text, st32 start_pos,
                             bool anchored_start, bool anchored_end);

  // Follow epsilon transitions from a single state
  void follow_epsilons(State *state, std::set<State *> &result_set,
                       std::vector<st32> &captures, bool at_start, bool at_end);

  // Get all states reachable via epsilon transitions from a set of states
  std::set<State *> follow_epsilons_from_set(const std::set<State *> &states,
                                             std::vector<st32> &captures,
                                             bool at_start, bool at_end);

  // Check if a ut8acter matches a state's condition
  bool state_matches(State *state, ut8 c);

  // Get all matching transitions from current states with a ut8acter
  std::set<State *> get_next_states(const std::set<State *> &current_states,
                                    ut8 c, std::vector<st32> &captures,
                                    bool at_start, bool at_end);
};

#endif // NFA_MATCHER_HPP
