#ifndef REGEX_POSTFIX_HPP
#define REGEX_POSTFIX_HPP

#include <RegexTokenizer.hpp>
#include <pz_cxx_std.hpp>
#include <pz_types.hpp>

/**
 * @brief Converts regex tokens from infix to postfix (RPN) form.
 *
 * This conversion is used as a preprocessing step before NFA construction.
 * The class is stateless and intended to be used via its static methods.
 */
class Postfix {
public:
  /**
   * @brief Convert an infix token sequence into postfix order.
   */
  static std::vector<Token> convert(const std::vector<Token> &infix);

private:
  /**
   * @brief Returns precedence of a regex operator token.
   */
  static st32 get_precedence(TokenType type);
};

#endif // REGEX_POSTFIX_HPP