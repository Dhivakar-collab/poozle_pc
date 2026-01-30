#include "RegexPostfix.hpp"
#include "pz_error.hpp"

st32 Postfix::get_precedence(TokenType type) {
  switch (type) {
  case TokenType::STAR:
  case TokenType::PLUS:
  case TokenType::QUESTION:
  case TokenType::QUANTIFIER_RANGE:
    return 3; // Unary postfix operators
  case TokenType::CONCAT:
    return 2; // Implicit concatenation
  case TokenType::ALTERNATION:
    return 1; // Lowest precedence
  default:
    return 0;
  }
}

std::vector<Token> Postfix::convert(const std::vector<Token> &infix) {
  std::vector<Token> postfix;
  std::stack<Token> operators;
  TokenType last_type = TokenType::END; // Tracks previous token for validation

  for (const auto &t : infix) {
    switch (t.type) {
    // Operands go directly to output
    case TokenType::LITERAL:
    case TokenType::DOT:
    case TokenType::CHAR_CLASS:
    case TokenType::CARET:
    case TokenType::DOLLAR:
      postfix.push_back(t);
      break;

    // '(' is pushed to operator stack and output (for NFA grouping)
    case TokenType::LPAREN: {
      postfix.push_back(t);
      operators.push(t);
      break;
    }

    // Pop operators until matching '(' is found
    case TokenType::RPAREN: {
      if (last_type == TokenType::LPAREN)
        PzError::report_error(PzError::PzErrorType::PZ_INVALID_INPUT,
                              "Empty Parentheses at position " +
                                  std::to_string(t.pos));
      while (!operators.empty() && operators.top().type != TokenType::LPAREN) {
        postfix.push_back(operators.top());
        operators.pop();
      }
      if (operators.empty())
        PzError::report_error(PzError::PzErrorType::PZ_INVALID_INPUT,
                              "Mismatched ')' at position " +
                                  std::to_string(t.pos));
      operators.pop(); // Discard '('
      postfix.push_back(t);
      break;
    }
    // Unary postfix operators must follow a valid expression
    case TokenType::STAR:
    case TokenType::PLUS:
    case TokenType::QUESTION:
    case TokenType::QUANTIFIER_RANGE:
      if (last_type != TokenType::LITERAL && last_type != TokenType::DOT &&
          last_type != TokenType::CHAR_CLASS &&
          last_type != TokenType::RPAREN) {
        PzError::report_error(PzError::PzErrorType::PZ_INVALID_INPUT,
                              "Quantifier used without a valid preceding "
                              "expression at position " +
                                  std::to_string(t.pos));
      }
      postfix.push_back(t);
      break;

    case TokenType::ALTERNATION:
      // '|' must separate two valid expressions
      if (last_type == TokenType::END || last_type == TokenType::LPAREN ||
          last_type == TokenType::ALTERNATION) {
        PzError::report_error(PzError::PzErrorType::PZ_INVALID_INPUT,
                              "Invalid '|' at position " +
                                  std::to_string(t.pos) +
                                  ". It must separate two expressions.");
      }
      goto push_operator;

    // Binary operators handled via precedence rules
    case TokenType::CONCAT:
    push_operator:
      while (!operators.empty() && operators.top().type != TokenType::LPAREN &&
             get_precedence(operators.top().type) >= get_precedence(t.type)) {
        postfix.push_back(operators.top());
        operators.pop();
      }
      operators.push(t);
      break;

    default:
      break;
    }

    if (t.type != TokenType::END)
      last_type = t.type;
  }

  // Pattern must not end with a binary operator
  if (last_type == TokenType::ALTERNATION || last_type == TokenType::CONCAT) {
    PzError::report_error(
        PzError::PzErrorType::PZ_INVALID_INPUT,
        "Trailing binary operator at end of pattern at position " +
            std::to_string(infix.back().pos));
  }

  // Drain remaining operators
  while (!operators.empty()) {
    if (operators.top().type == TokenType::LPAREN)
      PzError::report_error(PzError::PzErrorType::PZ_INVALID_INPUT,
                            "Unmatched '(' at position " +
                                std::to_string(operators.top().pos));
    postfix.push_back(operators.top());
    operators.pop();
  }

  return postfix;
}