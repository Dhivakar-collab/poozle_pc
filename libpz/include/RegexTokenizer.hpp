#ifndef REGEX_TOKENIZER_HPP
#define REGEX_TOKENIZER_HPP

#include <pz_cxx_std.hpp>

/**
 * @brief Types of tokens produced by the regex tokenizer.
 */
enum class TokenType {
  /** Literal character like 'a', 'b', etc. */
  LITERAL,

  /** '.' wildcard */
  DOT,

  /** '*' operator */
  STAR,

  /** '+' operator */
  PLUS,

  /** '?' operator */
  QUESTION,

  /** '|' alternation */
  ALTERNATION,

  /** '(' opening group */
  LPAREN,

  /** ')' closing group */
  RPAREN,

  /** '^' start anchor */
  CARET,

  /** '$' end anchor */
  DOLLAR,

  /** Character class: '[...]', \d, \w, \s, etc */
  CHAR_CLASS,

  /** Quantifier range: '{m,n}', '{m,}', '{m}' */
  QUANTIFIER_RANGE,

  /** End of pattern */
  END,

  /** Implicit concatenation */
  CONCAT
};

/**
 * @brief Represents a character range [lo, hi].
 */
struct CharRange {
  /** Lower bound */
  char lo;

  /** Upper bound */
  char hi;
};

/**
 * @brief A single token in the regex.
 */
struct Token {
  /** Token category */
  TokenType type;
  /** Position in pattern (for error reporting) */
  size_t pos;
  /** Group ID for parentheses */
  int group_id = -1;

  /** Literal character value */
  char literal = '\0';

  /** Whether character class is negated */
  bool negated = false;
  /** Character ranges for character class */
  std::vector<CharRange> ranges{};

  /** Minimum repetitions for quantifier */
  int min = 0;
  /** Maximum repetitions (-1 means unbounded) */
  int max = 0;
};

/**
 * @brief Converts a regex pattern into a sequence of tokens.
 */
class Tokenizer {
public:
  /**
   * @brief Construct tokenizer for a pattern.
   * @param pat Regex pattern.
   */
  explicit Tokenizer(std::string_view pat);

  /**
   * @brief Tokenize the entire pattern.
   * @return Vector of tokens ending with END token.
   */
  std::vector<Token> tokenize();

private:
  /** Input regex pattern */
  std::string_view pattern;
  /** Current cursor position */
  size_t i = 0;
  /** Counter for assigning group IDs */
  int group_counter = 0;
  /** Stack for nested group tracking */
  std::stack<int> group_stack;

  /** Peek next character without consuming */
  char peek() const;
  /** Consume next character */
  char get();
  /** Check for end of input */
  bool eof() const;

  /** Read next token */
  Token next_token();
  /** Read literal character */
  Token read_literal(char);
  /** Read escape sequence */
  Token read_escape();
  /** Read character class */
  Token read_char_class();
  /** Read quantifier range */
  Token read_quantifier();

  /** @brief Populates a token with ranges for \d, \w, \s, etc. */
  void add_shorthand_ranges(char, Token &);

  /** @brief Inserts implicit CONCAT tokens where concatenation occurs. */
  void add_concat_tokens(std::vector<Token> &);

  /** @brief Sorts and merges overlapping ranges for efficient NFA matching. */
  void normalize_ranges(std::vector<CharRange> &);
};

#endif // REGEX_TOKENIZER_HPP