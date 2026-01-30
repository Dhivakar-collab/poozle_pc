#ifndef NFA_BUILDER_HPP
#define NFA_BUILDER_HPP

#include <Nfa.hpp>

/**
 * @brief Builds an ε-NFA from a postfix regex token sequence.
 *
 * Implements Thompson-style construction to convert postfix regex tokens
 * into an NFA graph. All states created during construction are owned
 * internally and cleaned up automatically.
 */
class NfaBuilder {
public:
  /**
   * @brief Build an NFA from a postfix regex.
   *
   * The resulting NFA has a single accepting state of type
   * StateType::MATCH. The returned pointer refers to the start state.
   *
   * @param postfix Regex tokens in postfix (RPN) form.
   * @return Pointer to the start state of the constructed NFA.
   */
  State *build(const std::vector<Token> &postfix);

  /**
   * @brief Create a deep copy of an NFA fragment.
   *
   * Used for handling quantifiers that require duplication of subgraphs
   * (e.g. {m,n}, *, +).
   */
  Frag copy_fragment(Frag);

  /**
   * @brief Deep copy an NFA subgraph starting from a given state.
   *
   * Keeps a lookup map to avoid duplicating already-copied states.
   *
   * @param s Original state to copy.
   * @param lookup Map from original states to their copies.
   * @return Pointer to the copied state.
   */
  State *copy_state(State *, std::unordered_map<State *, State *> &);

private:
  /**
   * @brief Allocate a new NFA state and store it in the internal pool.
   *
   * Ownership is retained by the builder to ensure correct lifetime.
   */
  State *create_state(StateType type);

  /**
   * @brief Owns all NFA states created during construction.
   *
   * Ensures that all State objects remain valid for the lifetime
   * of the NfaBuilder and are automatically destroyed via RAII.
   */
  std::vector<std::unique_ptr<State>> state_pool;
};

#endif // NFA_BUILDER_HPP