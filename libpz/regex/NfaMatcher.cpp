#include "NfaMatcher.hpp"
#include <algorithm>

NfaMatcher::NfaMatcher(State *start_state) : start(start_state) {}

// Check if a character matches a state's matching condition
bool NfaMatcher::state_matches(State *state, char c) {
  switch (state->type) {
  case StateType::CHAR:
    return state->c == c;

  case StateType::DOT:
    return c != '\n'; // Dot matches anything except newline

  case StateType::CHAR_CLASS: {
    bool in_range = false;
    for (const auto &range : state->ranges) {
      if (c >= range.lo && c <= range.hi) {
        in_range = true;
        break;
      }
    }
    return state->negated ? !in_range : in_range;
  }

  default:
    return false;
  }
}

// Follow epsilon transitions from a single state, collecting all reachable
// states
void NfaMatcher::follow_epsilons(State *state, std::set<State *> &result_set,
                                 std::vector<int> &captures, bool at_start,
                                 bool at_end) {
  if (!state || result_set.count(state)) {
    return; // Already visited or null
  }

  // Check anchor constraints
  if (state->type == StateType::ANCHOR_START && !at_start) {
    return; // ^ anchor failed
  }
  if (state->type == StateType::ANCHOR_END && !at_end) {
    return; // $ anchor failed
  }

  result_set.insert(state);

  // Handle SAVE states (capture group boundaries)
  if (state->type == StateType::SAVE) {
    int save_id = state->save_id;
    if (save_id >= 0 && save_id < (int)captures.size()) {
      // Don't overwrite if already set in this path
      // This simple strategy takes the first match (greedy)
    }
  }

  // Follow epsilon transitions
  if (state->type == StateType::SPLIT) {
    // SPLIT has two epsilon transitions
    follow_epsilons(state->out, result_set, captures, at_start, at_end);
    follow_epsilons(state->out1, result_set, captures, at_start, at_end);
  } else if (state->type == StateType::SAVE) {
    // SAVE has one epsilon transition
    follow_epsilons(state->out, result_set, captures, at_start, at_end);
  } else if (state->type == StateType::ANCHOR_START ||
             state->type == StateType::ANCHOR_END) {
    // Anchors have one epsilon transition
    follow_epsilons(state->out, result_set, captures, at_start, at_end);
  }
  // CHAR, DOT, CHAR_CLASS, and MATCH don't have epsilon transitions
}

// Get all states reachable via epsilon transitions from a set of states
std::set<State *>
NfaMatcher::follow_epsilons_from_set(const std::set<State *> &states,
                                     std::vector<int> &captures, bool at_start,
                                     bool at_end) {
  std::set<State *> result;
  for (State *state : states) {
    follow_epsilons(state, result, captures, at_start, at_end);
  }
  return result;
}

// Get next states after consuming a character
std::set<State *>
NfaMatcher::get_next_states(const std::set<State *> &current_states, char c,
                            std::vector<int> &captures, bool at_start,
                            bool at_end) {
  std::set<State *> next_states;

  for (State *state : current_states) {
    if (state_matches(state, c)) {
      // This state matches the character, follow its transition
      if (state->out) {
        follow_epsilons(state->out, next_states, captures, at_start, at_end);
      }
    }
  }

  return next_states;
}

// Internal matching function
MatchResult NfaMatcher::match_internal(const std::string_view &text,
                                       int start_pos, bool anchored_start,
                                       bool anchored_end) {
  MatchResult result;

  // Initialize captures for potential groups (allocate enough space)
  std::vector<int> captures(100, -1); // Support up to 50 groups

  // Start with epsilon closure of start state
  std::set<State *> current_states;
  bool at_text_start = (start_pos == 0);
  follow_epsilons(start, current_states, captures, at_text_start, false);

  // Track positions where we've seen MATCH state
  int match_end = -1;
  std::vector<int> best_captures;

  // Check if we can match at the empty string (before consuming any characters)
  for (State *state : current_states) {
    if (state->type == StateType::MATCH) {
      match_end = start_pos;
      best_captures = captures;
      if (anchored_end) {
        // For fullmatch, we need to consume the entire string
        // So empty match only works if text is empty from start_pos
        if (start_pos >= (int)text.size()) {
          result.matched = true;
          result.start_pos = start_pos;
          result.end_pos = start_pos;
          return result;
        }
      } else {
        // For match/search, empty match is valid
        result.matched = true;
        result.start_pos = start_pos;
        result.end_pos = start_pos;
        // Continue to see if we can match more
      }
    }
  }

  // Process each character
  for (int i = start_pos; i < (int)text.size(); i++) {
    char c = text[i];
    bool at_end = (i + 1 == (int)text.size());

    // Get next states after consuming this character
    std::set<State *> next_states =
        get_next_states(current_states, c, captures, false, at_end);

    if (next_states.empty()) {
      break; // No more states to explore
    }

    current_states = next_states;

    // Update SAVE states with current position
    for (State *state : current_states) {
      if (state->type == StateType::SAVE) {
        int save_id = state->save_id;
        if (save_id >= 0 && save_id < (int)captures.size()) {
          captures[save_id] = i + 1; // Position after current character
        }
      }
    }

    // Check if any state is a MATCH state (greedy - continue to find longest
    // match)
    for (State *state : current_states) {
      if (state->type == StateType::MATCH) {
        match_end = i + 1;
        best_captures = captures;
      }
    }
  }

  // After consuming all characters, check for match at end
  if (match_end >= 0) {
    // For anchored_end (fullmatch), verify we consumed everything
    if (anchored_end && match_end != (int)text.size()) {
      result.matched = false;
      return result;
    }

    result.matched = true;
    result.start_pos = start_pos;
    result.end_pos = match_end;

    // Extract captures
    for (size_t j = 0; j < best_captures.size(); j += 2) {
      if (best_captures[j] >= 0 && best_captures[j + 1] >= 0) {
        result.captures.push_back({best_captures[j], best_captures[j + 1]});
      } else {
        result.captures.push_back({-1, -1});
      }
    }
  }

  return result;
}

// Match from the beginning (prefix match)
MatchResult NfaMatcher::match(const std::string_view &text) {
  return match_internal(text, 0, true, false);
}

// Find all non-overlapping matches in the text
// Similar to Python's re.findall() - returns all non-overlapping matches from
// left to right
std::vector<MatchResult> NfaMatcher::find_all(const std::string_view &text) {
  std::vector<MatchResult> matches;
  int pos = 0;

  while (pos <= (int)text.size()) {
    // Try to find a match starting at each position
    MatchResult result;

    // Initialize captures for potential groups
    std::vector<int> captures(100, -1);

    // Start with epsilon closure of start state
    std::set<State *> current_states;
    bool at_text_start = (pos == 0);
    follow_epsilons(start, current_states, captures, at_text_start, false);

    // Track best match found from this position
    int best_match_end = -1;
    std::vector<int> best_captures;

    // Check if we can match at the empty string
    for (State *state : current_states) {
      if (state->type == StateType::MATCH) {
        best_match_end = pos;
        best_captures = captures;
        break;
      }
    }

    // Process each character from this starting position
    for (int i = pos; i < (int)text.size(); i++) {
      char c = text[i];
      bool at_end = (i + 1 == (int)text.size());

      // Get next states after consuming this character
      std::set<State *> next_states =
          get_next_states(current_states, c, captures, false, at_end);

      if (next_states.empty()) {
        break; // No more states to explore from this position
      }

      current_states = next_states;

      // Update SAVE states with current position
      for (State *state : current_states) {
        if (state->type == StateType::SAVE) {
          int save_id = state->save_id;
          if (save_id >= 0 && save_id < (int)captures.size()) {
            captures[save_id] = i + 1;
          }
        }
      }

      // Check if any state is a MATCH state (greedy - prefer longest match)
      for (State *state : current_states) {
        if (state->type == StateType::MATCH) {
          best_match_end = i + 1;
          best_captures = captures;
        }
      }
    }

    // If we found a match from this position
    if (best_match_end >= 0) {
      result.matched = true;
      result.start_pos = pos;
      result.end_pos = best_match_end;

      // Extract captures
      for (size_t j = 0; j < best_captures.size(); j += 2) {
        if (best_captures[j] >= 0 && best_captures[j + 1] >= 0) {
          result.captures.push_back({best_captures[j], best_captures[j + 1]});
        } else {
          result.captures.push_back({-1, -1});
        }
      }

      matches.push_back(result);

      // Move past this match for next iteration (non-overlapping)
      // If match is empty (zero-length), advance by 1 to avoid infinite loop
      if (best_match_end == pos) {
        pos++;
      } else {
        pos = best_match_end;
      }
    } else {
      // No match found at this position, try next position
      pos++;
    }
  }

  return matches;
}

// Escape special regex characters in a string
// Similar to Python's re.escape() function
std::string NfaMatcher::escape(const std::string_view &text) {
  std::string result;
  result.reserve(text.size() * 2); // Reserve space to avoid reallocations

  for (char c : text) {
    // Check if character is a regex metacharacter
    switch (c) {
    // Operators
    case '.':
    case '*':
    case '+':
    case '?':
    case '|':

    // Grouping
    case '(':
    case ')':
    case '[':
    case ']':
    case '{':
    case '}':

    // Anchors
    case '^':
    case '$':

    // Escape character
    case '\\':

    // Character class special
    case '-':
      result += '\\';
      result += c;
      break;

    default:
      result += c;
      break;
    }
  }

  return result;
}
