#include "NfaBuilder.hpp"
#include "pz_error.hpp"

// Allocate a new NFA state, keep ownership in the builder(state pool),
// and return a raw pointer to the state.
State *NfaBuilder::create_state(StateType type) {
  state_pool.push_back(std::make_unique<State>(type));
  return state_pool.back().get();
}

// Create a deep copy of an NFA fragment.
// All states are duplicated except MATCH states, which are shared.
Frag NfaBuilder::copy_fragment(Frag original) {
  std::unordered_map<State *, State *>
      old_to_new; // stores the states we have already visited and its cloned
                  // copies
  State *new_start = copy_state(original.start, old_to_new);

  std::vector<State **> new_exits;

  // Traverse copied graph to collect dangling exits
  std::unordered_set<State *>
      visited; // Remember which states have been already visited
  std::stack<State *> s;
  s.push(new_start);
  while (!s.empty()) // Loop until there are no more states left to process
  {
    State *curr = s.top();
    s.pop();
    if (!curr || visited.count(curr))
      continue;
    visited.insert(curr);
    // If out is null, it's a dangling exit we need to patch later (Unpatched
    // primary exit)
    if (!curr->out && curr->type != StateType::MATCH) {
      new_exits.push_back(&curr->out);
    }
    // If out1 is null (and it's a SPLIT state), it's also an exit (Unpatched
    // secondary exit for SPLIT states)
    if (!curr->out1 && curr->type == StateType::SPLIT) {
      new_exits.push_back(&curr->out1);
    }

    if (curr->out)
      s.push(curr->out);
    if (curr->out1)
      s.push(curr->out1);
  }

  return Frag(new_start, new_exits);
}

// Recursively clone an NFA subgraph starting from state 's'.
// The 'lookup' map ensures that each original state is copied exactly once.
// This preserves shared structure and prevents infinite recursion on cycles.
// MATCH states are not duplicated: a copied fragment always reconnects to
// the same final MATCH state during patching.
State *NfaBuilder::copy_state(State *s,
                              std::unordered_map<State *, State *> &lookup) {

  // Null state or final MATCH state: return as-is
  if (!s || s->type == StateType::MATCH)
    return s;

  // If this state was already copied, reuse the existing clone
  if (lookup.count(s))
    return lookup[s];

  // Create a new state with the same semantic properties
  State *result = create_state(s->type);
  result->c = s->c;
  result->ranges = s->ranges;
  result->negated = s->negated;
  result->save_id = s->save_id;

  // Record the mapping before recursing to handle cycles correctly
  lookup[s] = result;

  // Recursively copy outgoing transitions
  result->out = copy_state(s->out, lookup);
  result->out1 = copy_state(s->out1, lookup);
  return result;
}

// Build an ε-NFA from a postfix (RPN) regex token sequence.
// The algorithm processes tokens left-to-right, maintaining a stack of
// NFA fragments. Each operator combines or transforms fragments according
// to standard Thompson construction rules. At the end, all dangling exits
// are patched to a single MATCH state.
State *NfaBuilder::build(const std::vector<Token> &postfix) {
  std::stack<Frag> stack;

  for (const auto &t : postfix) {
    switch (t.type) {

      // Atomic expressions:

    case TokenType::LITERAL: {
      State *s = create_state(StateType::CHAR);
      s->c = t.literal;
      stack.push(Frag(s));
      break;
    }
    case TokenType::DOT: {
      stack.push(Frag(create_state(StateType::DOT)));
      break;
    }
    case TokenType::CHAR_CLASS: {
      State *s = create_state(StateType::CHAR_CLASS);
      s->ranges = t.ranges;
      s->negated = t.negated;
      stack.push(Frag(s));
      break;
    }
    case TokenType::CARET: {
      stack.push(Frag(create_state(StateType::ANCHOR_START)));
      break;
    }
    case TokenType::DOLLAR: {
      stack.push(Frag(create_state(StateType::ANCHOR_END)));
      break;
    }

      // Capture groups:

    case TokenType::LPAREN: {
      State *s = create_state(StateType::SAVE);
      s->save_id = t.group_id * 2; // capture start (even)
      stack.push(Frag(s));
      break;
    }
    case TokenType::RPAREN: {
      // Create the save (end) state
      State *s = create_state(StateType::SAVE);
      s->save_id = t.group_id * 2 + 1; // capture end (odd)

      // Extract the content of the group along with save (start)
      Frag content = stack.top();
      stack.pop();
      Frag lparen_frag = stack.top();
      stack.pop();
      lparen_frag.patch(content.start);
      content.patch(s);

      // Push the whole fragment
      stack.push(Frag(lparen_frag.start, {&s->out}));
      break;
    }

      // Binary operators:

    case TokenType::CONCAT: {
      Frag e2 = stack.top();
      stack.pop();
      Frag e1 = stack.top();
      stack.pop();
      e1.patch(e2.start);
      stack.push(Frag(e1.start, e2.out_ptrs));
      break;
    }
    case TokenType::ALTERNATION: {
      Frag e2 = stack.top();
      stack.pop();
      Frag e1 = stack.top();
      stack.pop();
      State *s = create_state(StateType::SPLIT);
      s->out = e1.start;
      s->out1 = e2.start;
      // Combine dangling exits from both branches
      std::vector<State **> combined = e1.out_ptrs;
      combined.insert(combined.end(), e2.out_ptrs.begin(), e2.out_ptrs.end());
      stack.push(Frag(s, combined));
      break;
    }

      // Unary operators:

    case TokenType::STAR: {
      Frag e = stack.top();
      stack.pop();
      State *s = create_state(StateType::SPLIT);
      s->out = e.start; // Loop back into the expression
      e.patch(s);       // The expression's end loops back to the split
      stack.push(Frag(s, {&s->out1})); // out1 is the escape route
      break;
    }
    case TokenType::PLUS: {
      Frag e = stack.top();
      stack.pop();
      State *s = create_state(StateType::SPLIT);
      s->out = e.start; // Loop back
      e.patch(s);       // Connect expression end to split
      stack.push(Frag(e.start, {&s->out1}));
      break;
    }
    case TokenType::QUESTION: {
      Frag e = stack.top();
      stack.pop();
      State *s = create_state(StateType::SPLIT);
      s->out = e.start; // Option 1: match the expression
      // Option 2: skip the expression (out1)
      std::vector<State **> exits = e.out_ptrs;
      exits.push_back(&s->out1);
      stack.push(Frag(s, exits));
      break;
    }

      // Bounded repetition:

    case TokenType::QUANTIFIER_RANGE: {
      Frag e = stack.top();
      stack.pop();

      // i) Handle the mandatory part (m)
      // Initialize 'mandatory' with an immediately-invoked lambda (no valid
      // default state).
      Frag mandatory = [&]() {
        if (t.min == 0) {
          State *eps = create_state(StateType::SPLIT);
          return Frag(eps, {&eps->out});
        } else {
          return copy_fragment(e); // Use the first one as the base
        }
      }();
      // If min > 1, append the necessary copies
      for (int i = 1; i < t.min; i++) {
        Frag next_copy = copy_fragment(e);
        mandatory.patch(next_copy.start);
        mandatory = Frag(mandatory.start, next_copy.out_ptrs);
      }

      // ii) Handle the optional part (n - m) or infinite (m, )
      if (t.max == -1) { // {m,}
        State *s = create_state(StateType::SPLIT);
        Frag loop_part = copy_fragment(e);

        s->out = loop_part.start;
        loop_part.patch(s);

        mandatory.patch(s);
        stack.push(Frag(mandatory.start, {&s->out1}));
      } else if (t.max > t.min) { // {m,n}
        // Build a chain of optional fragments, each one guarded by a SPLIT that
        // can either take the repetition or skip it and move on
        Frag optional_chain = mandatory;
        std::vector<State **> all_exits;

        for (int i = 0; i < (t.max - t.min); i++) {
          Frag next_opt = copy_fragment(e);
          State *s = create_state(StateType::SPLIT);

          s->out = next_opt.start;
          optional_chain.patch(s);

          // Collect exits from the skip path
          all_exits.push_back(&s->out1);

          optional_chain = Frag(next_opt.start, next_opt.out_ptrs);
        }
        // Add exits from the last repetition: if all optional parts are taken,
        // the match can continue after the final copied fragment.
        all_exits.insert(all_exits.end(), optional_chain.out_ptrs.begin(),
                         optional_chain.out_ptrs.end());
        stack.push(Frag(mandatory.start, all_exits));
      } else { // {m}
        stack.push(mandatory);
      }
      break;
    }
    default:
      break;
    }
  }

  // Empty regex produces an ε-NFA (No fragments)
  if (stack.empty()) {
    State *s = create_state(StateType::SPLIT);
    stack.push(Frag(s));
  }

  // Implicit concatenation of remaining fragments
  while (stack.size() > 1) {
    Frag e2 = stack.top();
    stack.pop();
    Frag e1 = stack.top();
    stack.pop();
    e1.patch(e2.start);
    stack.push(Frag(e1.start, e2.out_ptrs));
  }

  // Patch all remaining exits to the final MATCH state
  Frag final_frag = stack.top();
  stack.pop();
  State *match_state = create_state(StateType::MATCH);
  final_frag.patch(match_state);

  return final_frag.start;
}