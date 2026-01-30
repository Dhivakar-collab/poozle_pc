#ifndef NFA_HPP
#define NFA_HPP

#include <RegexTokenizer.hpp>
#include <pz_cxx_std.hpp>
#include <pz_types.hpp>

/**
 * @brief Types of NFA states used in regex matching.
 */
enum class StateType {
  /** Match a single literal character */
  CHAR,

  /** Match any character (.) */
  DOT,

  /** Match a character class ([...]) */
  CHAR_CLASS,

  /** Accepting (final) state */
  MATCH,

  /** ε-transition with two outgoing branches */
  SPLIT,

  /** Save input position (for capture groups) */
  SAVE,

  /** Start-of-input anchor (^) */
  ANCHOR_START,

  /** End-of-input anchor ($) */
  ANCHOR_END
};

/**
 * @brief Represents a single state in the NFA.
 */
struct State {
  StateType type;

  /** Literal character to match (valid only for CHAR states, unspecified
   * otherwise). */
  ut8 c;

  /** Capture group identifier (used by SAVE states to store input positions).
   */
  st32 save_id = -1;
  // Even IDs represent group start, odd IDs represent group end.

  /** Character ranges for CHAR_CLASS states. */
  std::vector<CharRange> ranges;
  bool negated = false;

  /** Primary outgoing transition. */
  State *out = nullptr;

  /** Secondary outgoing transition (used only by SPLIT states). */
  State *out1 = nullptr;

  /**
   * @brief Marker used during NFA simulation.
   *
   * Prevents revisiting the same state multiple times in a single step,
   * avoiding duplicate work and infinite ε-transition loops.
   */
  st32 last_list = -1;
  // Marks whether this state has already been added to the current
  // active-states list, preventing duplicate entries and infinite ε-transition
  // loops

  State(StateType t) : type(t) {}
};

/**
 * @brief Represents a partially constructed NFA fragment.
 *
 * A fragment consists of:
 *  - a start state
 *  - a list of dangling outgoing transitions that must be patched later
 */
struct Frag {
  State *start;

  /** Addresses of state pointers that need to be connected later. */
  std::vector<State **> out_ptrs;

  /**
   * @brief Construct a fragment with a single dangling exit.
   */
  Frag(State *s) : start(s) { out_ptrs.push_back(&s->out); }

  /**
   * @brief Construct a fragment with multiple dangling exits.
   */
  Frag(State *s, std::vector<State **> out) : start(s), out_ptrs(out) {}

  /**
   * @brief Patch all dangling exits to point to the given state.
   */
  void patch(State *s) {
    for (auto &ptr : out_ptrs) {
      if (ptr &&
          !*ptr) { // Only patch if the pointer exists and is currently null
        *ptr = s;
      }
    }
  }
};

#endif // NFA_HPP