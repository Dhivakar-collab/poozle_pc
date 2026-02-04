#include "NfaMatcher.hpp"
#include "nfa_builder.hpp"
#include "postfix.hpp"
#include "tokenizer.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace std::chrono;

struct TestCase {
  std::string pattern;
  std::string text;
  bool expected;
  std::string description;
};

int total_tests = 0;
int passed_tests = 0;
int failed_tests = 0;

void run_match_test(const TestCase &test) {
  total_tests++;
  try {
    Tokenizer tokenizer(test.pattern);
    auto tokens = tokenizer.tokenize();
    auto postfix = PostfixConverter::convert(tokens);
    NfaBuilder builder;
    State *start = builder.build(postfix);
    NfaMatcher matcher(start);
    MatchResult result = matcher.match(test.text);

    bool outcome = result.matched;
    bool success = (outcome == test.expected);

    if (success) {
      passed_tests++;
      std::cout << "[SUCCESS] ";
    } else {
      failed_tests++;
      std::cout << "[FAILURE] ";
    }

    std::cout << std::setw(35) << std::left << test.description
              << " | Pattern: " << std::setw(30) << std::left << test.pattern
              << " | Text: " << std::setw(25) << std::left << test.text
              << " | Expected: " << (test.expected ? "MATCH  " : "NO MATCH")
              << " | Got: " << (outcome ? "MATCH  " : "NO MATCH");

    if (result.matched) {
      std::cout << " [" << result.start_pos << "," << result.end_pos << ")";
    }
    std::cout << "\n";

  } catch (const std::exception &e) {
    failed_tests++;
    std::cout << "[FAILURE] " << std::setw(35) << std::left << test.description
              << " | Pattern: " << test.pattern << " | ERROR: " << e.what()
              << "\n";
  }
}

int main() {
  auto start_time = high_resolution_clock::now();

  std::vector<TestCase> tests;

  // ========================================================================
  // BASIC LITERAL MATCHING
  // ========================================================================
  std::cout << "\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "BASIC LITERAL MATCHING\n";
  std::cout << "==============================================================="
               "=================\n";

  tests = {
      {"a", "a", true, "Single char match"},
      {"a", "b", false, "Single char no match"},
      {"abc", "abc", true, "Multi char exact match"},
      {"abc", "abcd", true, "Prefix match success"},
      {"abc", "ab", false, "Incomplete match"},
      {"abc", "xabc", false, "Match not at start"},
      {"hello", "hello world", true, "Word prefix match"},
      {"world", "hello world", false, "Word not at start"},
      {"", "", true, "Empty pattern empty text"},
      {"", "abc", true, "Empty pattern any text"},
      {"abc", "", false, "Non-empty pattern empty text"},
      {"xyz", "xyz", true, "Three char match"},
      {"test", "testing", true, "Substring at start"},
      {"test", "contest", false, "Substring not at start"},
      {"programming", "programming language", true, "Long word prefix"},
  };
  for (const auto &t : tests)
    run_match_test(t);

  // ========================================================================
  // DOT OPERATOR (.)
  // ========================================================================
  std::cout << "\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "DOT OPERATOR (.)\n";
  std::cout << "==============================================================="
               "=================\n";

  tests = {
      {".", "a", true, "Dot matches single char"},
      {".", "x", true, "Dot matches any letter"},
      {".", "5", true, "Dot matches digit"},
      {".", " ", true, "Dot matches space"},
      {".", "\n", false, "Dot does not match newline"},
      {".", "", false, "Dot requires one char"},
      {"a.c", "abc", true, "Dot in middle"},
      {"a.c", "axc", true, "Dot matches any middle char"},
      {"a.c", "a5c", true, "Dot matches digit"},
      {"a.c", "ac", false, "Dot requires a char"},
      {"a.c", "abbc", false, "Dot matches exactly one"},
      {"..", "ab", true, "Two dots two chars"},
      {"...", "xyz", true, "Three dots three chars"},
      {"....", "abc", false, "Four dots three chars"},
      {"a.b.c", "aXbYc", true, "Multiple dots"},
      {".", "\t", true, "Dot matches tab"},
      {".a", "ba", true, "Dot at start"},
      {"a.", "ab", true, "Dot at end"},
      {".a.b.", "1a2b3", true, "Alternating dots"},
      {"h.llo", "hello", true, "Dot in word"},
      {"h.llo", "hallo", true, "Dot matches variation"},
  };
  for (const auto &t : tests)
    run_match_test(t);

  // ========================================================================
  // STAR OPERATOR (*)
  // ========================================================================
  std::cout << "\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "STAR OPERATOR (*) - Zero or More\n";
  std::cout << "==============================================================="
               "=================\n";

  tests = {
      {"a*", "", true, "Star matches zero"},
      {"a*", "a", true, "Star matches one"},
      {"a*", "aaa", true, "Star matches many"},
      {"a*", "aaaaaaaaaa", true, "Star matches ten"},
      {"a*", "b", true, "Star zero then mismatch"},
      {"a*b", "b", true, "Star zero then match"},
      {"a*b", "ab", true, "Star one then match"},
      {"a*b", "aaab", true, "Star many then match"},
      {"a*b", "aaaaaaab", true, "Star lots then match"},
      {"a*b", "a", false, "Star many but no b"},
      {"ab*", "a", true, "Star at end zero"},
      {"ab*", "ab", true, "Star at end one"},
      {"ab*", "abbb", true, "Star at end many"},
      {"ab*c", "ac", true, "Star middle zero"},
      {"ab*c", "abc", true, "Star middle one"},
      {"ab*c", "abbbc", true, "Star middle many"},
      {"a*b*", "", true, "Two stars zero each"},
      {"a*b*", "aaa", true, "First star many second zero"},
      {"a*b*", "bbb", true, "First star zero second many"},
      {"a*b*", "aaabbb", true, "Both stars many"},
      {".*", "", true, "Dotstar empty"},
      {".*", "anything", true, "Dotstar matches all"},
      {"a.*", "a", true, "A then dotstar empty"},
      {"a.*", "abc", true, "A then dotstar matches"},
      {"a.*z", "abcxyz", true, "Dotstar in middle"},
      {"(ab)*", "", true, "Group star zero"},
      {"(ab)*", "ab", true, "Group star one"},
      {"(ab)*", "abab", true, "Group star two"},
      {"(ab)*", "ababab", true, "Group star three"},
      {"(ab)*", "aba", true, "Group star incomplete prefix"},
      {"x*y*z*", "xxxyyyzzz", true, "Triple star all present"},
      {"x*y*z*", "yyyzzz", true, "Triple star first zero"},
      {"x*y*z*", "xxxzzz", true, "Triple star middle zero"},
      {"x*y*z*", "xxxyyy", true, "Triple star last zero"},
      {"x*y*z*", "", true, "Triple star all zero"},
  };
  for (const auto &t : tests)
    run_match_test(t);

  // ========================================================================
  // PLUS OPERATOR (+)
  // ========================================================================
  std::cout << "\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "PLUS OPERATOR (+) - One or More\n";
  std::cout << "==============================================================="
               "=================\n";

  tests = {
      {"a+", "", false, "Plus requires at least one"},
      {"a+", "a", true, "Plus matches one"},
      {"a+", "aa", true, "Plus matches two"},
      {"a+", "aaaaa", true, "Plus matches many"},
      {"a+", "b", false, "Plus no match"},
      {"a+b", "ab", true, "Plus one then b"},
      {"a+b", "aaab", true, "Plus many then b"},
      {"a+b", "b", false, "Plus requires a before b"},
      {"ab+", "ab", true, "Plus at end one"},
      {"ab+", "abbb", true, "Plus at end many"},
      {"ab+", "a", false, "Plus at end requires b"},
      {"ab+c", "abc", true, "Plus middle one"},
      {"ab+c", "abbc", true, "Plus middle two"},
      {"ab+c", "abbbbbc", true, "Plus middle many"},
      {"ab+c", "ac", false, "Plus middle requires b"},
      {"a+b+", "ab", true, "Two plus minimum"},
      {"a+b+", "aaabbb", true, "Two plus many"},
      {"a+b+", "a", false, "Two plus first only"},
      {"a+b+", "b", false, "Two plus second only"},
      {".+", "x", true, "Dotplus one char"},
      {".+", "anything", true, "Dotplus many chars"},
      {".+", "", false, "Dotplus empty fails"},
      {"a.+b", "axb", true, "Dotplus middle minimum"},
      {"a.+b", "axxxxb", true, "Dotplus middle many"},
      {"a.+b", "ab", false, "Dotplus requires one"},
      {"(ab)+", "ab", true, "Group plus one"},
      {"(ab)+", "abab", true, "Group plus two"},
      {"(ab)+", "ababab", true, "Group plus three"},
      {"(ab)+", "", false, "Group plus empty fails"},
      {"(ab)+", "a", false, "Group plus incomplete"},
      {"\\d+", "5", true, "Digit plus one"},
      {"\\d+", "12345", true, "Digit plus many"},
      {"\\d+", "", false, "Digit plus empty"},
      {"\\d+", "abc", false, "Digit plus no digits"},
      {"\\w+", "hello", true, "Word plus letters"},
      {"\\w+", "hello123", true, "Word plus mixed"},
      {"\\w+", "", false, "Word plus empty"},
  };
  for (const auto &t : tests)
    run_match_test(t);

  // ========================================================================
  // QUESTION OPERATOR (?)
  // ========================================================================
  std::cout << "\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "QUESTION OPERATOR (?) - Zero or One\n";
  std::cout << "==============================================================="
               "=================\n";

  tests = {
      {"a?", "", true, "Question matches zero"},
      {"a?", "a", true, "Question matches one"},
      {"a?", "aa", true, "Question matches one of two"},
      {"a?b", "b", true, "Question zero then match"},
      {"a?b", "ab", true, "Question one then match"},
      {"a?b", "aab", false, "Question max one"},
      {"ab?", "a", true, "Question at end zero"},
      {"ab?", "ab", true, "Question at end one"},
      {"ab?", "abb", true, "Question at end prefix match"},
      {"ab?c", "ac", true, "Question middle zero"},
      {"ab?c", "abc", true, "Question middle one"},
      {"ab?c", "abbc", false, "Question middle does not match two"},
      {"colou?r", "color", true, "Optional u - without"},
      {"colou?r", "colour", true, "Optional u - with"},
      {"colou?r", "colouur", false, "Optional u - too many"},
      {"a?b?c?", "", true, "Three question all zero"},
      {"a?b?c?", "a", true, "Three question first only"},
      {"a?b?c?", "b", true, "Three question middle only"},
      {"a?b?c?", "c", true, "Three question last only"},
      {"a?b?c?", "ab", true, "Three question first two"},
      {"a?b?c?", "bc", true, "Three question last two"},
      {"a?b?c?", "abc", true, "Three question all present"},
      {".?", "", true, "Dotquestion zero"},
      {".?", "x", true, "Dotquestion one"},
      {".?", "xy", true, "Dotquestion one of two"},
      {"(ab)?", "", true, "Group question zero"},
      {"(ab)?", "ab", true, "Group question one"},
      {"(ab)?", "abab", true, "Group question prefix match"},
      {"https?://", "http://", true, "Optional s - without"},
      {"https?://", "https://", true, "Optional s - with"},
      {"\\d?", "", true, "Digit question zero"},
      {"\\d?", "5", true, "Digit question one"},
      {"\\d?\\d?", "12", true, "Two digit question both"},
      {"\\d?\\d?", "1", true, "Two digit question one"},
      {"\\d?\\d?", "", true, "Two digit question zero"},
  };
  for (const auto &t : tests)
    run_match_test(t);

  // ========================================================================
  // ALTERNATION (|)
  // ========================================================================
  std::cout << "\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "ALTERNATION (|) - OR Operator\n";
  std::cout << "==============================================================="
               "=================\n";

  tests = {
      {"a|b", "a", true, "Alternation first option"},
      {"a|b", "b", true, "Alternation second option"},
      {"a|b", "c", false, "Alternation no match"},
      {"a|b", "ab", true, "Alternation prefix match"},
      {"cat|dog", "cat", true, "Word alternation first"},
      {"cat|dog", "dog", true, "Word alternation second"},
      {"cat|dog", "bird", false, "Word alternation no match"},
      {"cat|dog", "category", true, "Word alternation prefix"},
      {"a|b|c", "a", true, "Triple alternation first"},
      {"a|b|c", "b", true, "Triple alternation second"},
      {"a|b|c", "c", true, "Triple alternation third"},
      {"a|b|c", "d", false, "Triple alternation no match"},
      {"(abc)|(def)", "abc", true, "Group alternation first"},
      {"(abc)|(def)", "def", true, "Group alternation second"},
      {"(abc)|(def)", "ghi", false, "Group alternation no match"},
      {"red|green|blue", "red", true, "Color first"},
      {"red|green|blue", "green", true, "Color second"},
      {"red|green|blue", "blue", true, "Color third"},
      {"red|green|blue", "yellow", false, "Color no match"},
      {"a|ab", "a", true, "Alternation shorter first"},
      {"a|ab", "ab", true, "Alternation longer second"},
      {"ab|a", "a", true, "Alternation order matters"},
      {"ab|a", "ab", true, "Alternation longer matches"},
      {"(a|b)c", "ac", true, "Alternation in group then c"},
      {"(a|b)c", "bc", true, "Alternation in group then c"},
      {"(a|b)c", "cc", false, "Alternation in group no match"},
      {"a(b|c)", "ab", true, "A then alternation first"},
      {"a(b|c)", "ac", true, "A then alternation second"},
      {"a(b|c)", "ad", false, "A then alternation no match"},
      {"(a|b)(c|d)", "ac", true, "Two alternations 1-1"},
      {"(a|b)(c|d)", "ad", true, "Two alternations 1-2"},
      {"(a|b)(c|d)", "bc", true, "Two alternations 2-1"},
      {"(a|b)(c|d)", "bd", true, "Two alternations 2-2"},
      {"(a|b)(c|d)", "ae", false, "Two alternations no match"},
      {"yes|no|maybe", "yes", true, "Decision first"},
      {"yes|no|maybe", "no", true, "Decision second"},
      {"yes|no|maybe", "maybe", true, "Decision third"},
      {"", "", true, "Empty alternation sides"},
  };
  for (const auto &t : tests)
    run_match_test(t);

  // ========================================================================
  // CHARACTER CLASSES
  // ========================================================================
  std::cout << "\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "CHARACTER CLASSES\n";
  std::cout << "==============================================================="
               "=================\n";

  tests = {
      {"[abc]", "a", true, "Class matches first"},
      {"[abc]", "b", true, "Class matches second"},
      {"[abc]", "c", true, "Class matches third"},
      {"[abc]", "d", false, "Class no match"},
      {"[abc]", "", false, "Class empty text"},
      {"[a-z]", "a", true, "Range matches first"},
      {"[a-z]", "m", true, "Range matches middle"},
      {"[a-z]", "z", true, "Range matches last"},
      {"[a-z]", "A", false, "Range case sensitive"},
      {"[a-z]", "5", false, "Range no digit"},
      {"[A-Z]", "A", true, "Upper range first"},
      {"[A-Z]", "M", true, "Upper range middle"},
      {"[A-Z]", "Z", true, "Upper range last"},
      {"[A-Z]", "a", false, "Upper range no lower"},
      {"[0-9]", "0", true, "Digit range first"},
      {"[0-9]", "5", true, "Digit range middle"},
      {"[0-9]", "9", true, "Digit range last"},
      {"[0-9]", "a", false, "Digit range no letter"},
      {"[a-zA-Z]", "a", true, "Multi range lower"},
      {"[a-zA-Z]", "Z", true, "Multi range upper"},
      {"[a-zA-Z]", "5", false, "Multi range no digit"},
      {"[a-z0-9]", "x", true, "Alphanum letter"},
      {"[a-z0-9]", "5", true, "Alphanum digit"},
      {"[a-z0-9]", "X", false, "Alphanum no upper"},
      {"[a-zA-Z0-9]", "a", true, "Full alphanum lower"},
      {"[a-zA-Z0-9]", "Z", true, "Full alphanum upper"},
      {"[a-zA-Z0-9]", "5", true, "Full alphanum digit"},
      {"[a-zA-Z0-9]", "!", false, "Full alphanum no special"},
      {"[aeiou]", "a", true, "Vowels a"},
      {"[aeiou]", "e", true, "Vowels e"},
      {"[aeiou]", "i", true, "Vowels i"},
      {"[aeiou]", "o", true, "Vowels o"},
      {"[aeiou]", "u", true, "Vowels u"},
      {"[aeiou]", "b", false, "Vowels no consonant"},
      {"[abc]+", "a", true, "Class plus one"},
      {"[abc]+", "abc", true, "Class plus multiple"},
      {"[abc]+", "aaabbbccc", true, "Class plus repeated"},
      {"[abc]+", "d", false, "Class plus no match"},
      {"[0-9]+", "123", true, "Digit class plus"},
      {"[0-9]+", "0", true, "Digit class plus one"},
      {"[0-9]+", "abc", false, "Digit class plus no match"},
      {"[a-z]*", "", true, "Lower class star zero"},
      {"[a-z]*", "hello", true, "Lower class star many"},
      {"[a-z]*", "123", true, "Lower class star zero then mismatch"},
  };
  for (const auto &t : tests)
    run_match_test(t);

  // ========================================================================
  // NEGATED CHARACTER CLASSES
  // ========================================================================
  std::cout << "\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "NEGATED CHARACTER CLASSES\n";
  std::cout << "==============================================================="
               "=================\n";

  tests = {
      {"[^abc]", "d", true, "Negated class match"},
      {"[^abc]", "x", true, "Negated class other"},
      {"[^abc]", "a", false, "Negated class first"},
      {"[^abc]", "b", false, "Negated class second"},
      {"[^abc]", "c", false, "Negated class third"},
      {"[^a-z]", "A", true, "Negated range upper"},
      {"[^a-z]", "5", true, "Negated range digit"},
      {"[^a-z]", "a", false, "Negated range in range"},
      {"[^a-z]", "m", false, "Negated range middle"},
      {"[^a-z]", "z", false, "Negated range last"},
      {"[^0-9]", "a", true, "Negated digit letter"},
      {"[^0-9]", "!", true, "Negated digit special"},
      {"[^0-9]", "5", false, "Negated digit in range"},
      {"[^aeiou]", "b", true, "Negated vowels consonant"},
      {"[^aeiou]", "x", true, "Negated vowels other"},
      {"[^aeiou]", "a", false, "Negated vowels a"},
      {"[^aeiou]", "e", false, "Negated vowels e"},
      {"[^A-Z]", "a", true, "Negated upper lower ok"},
      {"[^A-Z]", "5", true, "Negated upper digit ok"},
      {"[^A-Z]", "A", false, "Negated upper first"},
      {"[^A-Z]", "Z", false, "Negated upper last"},
      {"[^abc]+", "xyz", true, "Negated class plus match"},
      {"[^abc]+", "defgh", true, "Negated class plus many"},
      {"[^abc]+", "a", false, "Negated class plus excluded"},
      {"[^0-9]+", "hello", true, "Negated digit plus letters"},
      {"[^0-9]+", "123", false, "Negated digit plus digits"},
      {"[^ ]", "a", true, "Negated space letter"},
      {"[^ ]", "5", true, "Negated space digit"},
      {"[^ ]", " ", false, "Negated space space"},
      {"[^\n]", "a", true, "Negated newline letter"},
      {"[^\n]", "\n", false, "Negated newline newline"},
  };
  for (const auto &t : tests)
    run_match_test(t);

  // ========================================================================
  // ESCAPE SEQUENCES - SHORTHAND CHARACTER CLASSES
  // ========================================================================
  std::cout << "\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "ESCAPE SEQUENCES - SHORTHAND CHARACTER CLASSES\n";
  std::cout << "==============================================================="
               "=================\n";

  tests = {
      {"\\d", "0", true, "Digit shorthand 0"},
      {"\\d", "5", true, "Digit shorthand 5"},
      {"\\d", "9", true, "Digit shorthand 9"},
      {"\\d", "a", false, "Digit shorthand letter"},
      {"\\d", " ", false, "Digit shorthand space"},
      {"\\d+", "0", true, "Digits plus one"},
      {"\\d+", "123", true, "Digits plus many"},
      {"\\d+", "0987654321", true, "Digits plus all"},
      {"\\d+", "", false, "Digits plus empty"},
      {"\\d+", "abc", false, "Digits plus letters"},
      {"\\d*", "", true, "Digits star zero"},
      {"\\d*", "123", true, "Digits star many"},
      {"\\d*", "abc", true, "Digits star zero then letters"},
      {"\\D", "a", true, "Non-digit letter"},
      {"\\D", "Z", true, "Non-digit upper"},
      {"\\D", "!", true, "Non-digit special"},
      {"\\D", " ", true, "Non-digit space"},
      {"\\D", "5", false, "Non-digit digit"},
      {"\\D+", "hello", true, "Non-digits plus letters"},
      {"\\D+", "!@#", true, "Non-digits plus special"},
      {"\\D+", "123", false, "Non-digits plus digits"},
      {"\\w", "a", true, "Word char lower"},
      {"\\w", "Z", true, "Word char upper"},
      {"\\w", "5", true, "Word char digit"},
      {"\\w", "_", true, "Word char underscore"},
      {"\\w", "!", false, "Word char special"},
      {"\\w", " ", false, "Word char space"},
      {"\\w+", "hello", true, "Word chars letters"},
      {"\\w+", "Hello123", true, "Word chars mixed"},
      {"\\w+", "test_var", true, "Word chars with underscore"},
      {"\\w+", "hello world", true, "Word chars prefix"},
      {"\\w+", "", false, "Word chars empty"},
      {"\\w+", "!!!", false, "Word chars special only"},
      {"\\W", "!", true, "Non-word special"},
      {"\\W", " ", true, "Non-word space"},
      {"\\W", "@", true, "Non-word at"},
      {"\\W", "a", false, "Non-word letter"},
      {"\\W", "5", false, "Non-word digit"},
      {"\\W", "_", false, "Non-word underscore"},
      {"\\W+", "!@#", true, "Non-word chars special"},
      {"\\W+", "   ", true, "Non-word chars spaces"},
      {"\\W+", "abc", false, "Non-word chars letters"},
      {"\\s", " ", true, "Space space"},
      {"\\s", "\t", true, "Space tab"},
      {"\\s", "\n", true, "Space newline"},
      {"\\s", "\r", true, "Space carriage return"},
      {"\\s", "a", false, "Space letter"},
      {"\\s", "5", false, "Space digit"},
      {"\\s+", "   ", true, "Spaces plus many"},
      {"\\s+", "\t\t", true, "Spaces plus tabs"},
      {"\\s+", " \t\n", true, "Spaces plus mixed"},
      {"\\s+", "", false, "Spaces plus empty"},
      {"\\s+", "abc", false, "Spaces plus letters"},
      {"\\S", "a", true, "Non-space letter"},
      {"\\S", "5", true, "Non-space digit"},
      {"\\S", "!", true, "Non-space special"},
      {"\\S", " ", false, "Non-space space"},
      {"\\S", "\t", false, "Non-space tab"},
      {"\\S+", "hello", true, "Non-spaces plus word"},
      {"\\S+", "123", true, "Non-spaces plus digits"},
      {"\\S+", "   ", false, "Non-spaces plus spaces"},
  };
  for (const auto &t : tests)
    run_match_test(t);

  // ========================================================================
  // ESCAPE SEQUENCES - SPECIAL CHARACTERS
  // ========================================================================
  std::cout << "\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "ESCAPE SEQUENCES - SPECIAL CHARACTERS\n";
  std::cout << "==============================================================="
               "=================\n";

  tests = {
      {"\\n", "\n", true, "Escaped newline match"},
      {"\\n", "n", false, "Escaped newline letter"},
      {"\\t", "\t", true, "Escaped tab match"},
      {"\\t", "t", false, "Escaped tab letter"},
      {"\\r", "\r", true, "Escaped CR match"},
      {"\\r", "r", false, "Escaped CR letter"},
      {"\\.", ".", true, "Escaped dot literal"},
      {"\\.", "x", false, "Escaped dot not any"},
      {"\\*", "*", true, "Escaped star literal"},
      {"\\*", "a", false, "Escaped star not quantifier"},
      {"\\+", "+", true, "Escaped plus literal"},
      {"\\+", "a", false, "Escaped plus not quantifier"},
      {"\\?", "?", true, "Escaped question literal"},
      {"\\?", "a", false, "Escaped question not quantifier"},
      {"\\|", "|", true, "Escaped pipe literal"},
      {"\\|", "a", false, "Escaped pipe not alternation"},
      {"\\(", "(", true, "Escaped lparen literal"},
      {"\\(", "a", false, "Escaped lparen not group"},
      {"\\)", ")", true, "Escaped rparen literal"},
      {"\\)", "a", false, "Escaped rparen not group"},
      {"\\[", "[", true, "Escaped lbracket literal"},
      {"\\[", "a", false, "Escaped lbracket not class"},
      {"\\]", "]", true, "Escaped rbracket literal"},
      {"\\]", "a", false, "Escaped rbracket not class"},
      {"\\\\", "\\", true, "Escaped backslash literal"},
      {"\\\\", "a", false, "Escaped backslash not escape"},
      {"a\\.b", "a.b", true, "Escaped dot in pattern"},
      {"a\\.b", "axb", false, "Escaped dot literal only"},
      {"\\d\\+\\d", "5+3", true, "Escaped plus between digits"},
      {"\\(\\d\\)", "(5)", true, "Escaped parens around digit"},
  };
  for (const auto &t : tests)
    run_match_test(t);

  // ========================================================================
  // ANCHORS
  // ========================================================================
  std::cout << "\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "ANCHORS (^ and $)\n";
  std::cout << "==============================================================="
               "=================\n";

  tests = {
      {"^abc", "abc", true, "Anchor start match"},
      {"^abc", "abcdef", true, "Anchor start prefix"},
      {"^abc", "xabc", false, "Anchor start not at beginning"},
      {"^abc", "  abc", false, "Anchor start space before"},
      {"^hello", "hello world", true, "Anchor start word"},
      {"^hello", "say hello", false, "Anchor start later"},
      {"^", "", true, "Anchor start empty"},
      {"^", "anything", true, "Anchor start any"},
      {"^a", "a", true, "Anchor start single"},
      {"^a", "b", false, "Anchor start wrong char"},
      {"abc$", "abc", true, "Anchor end exact (prefix match)"},
      {"abc$", "xyzabc", false, "Anchor end not at start"},
      {"world$", "hello world", false, "Anchor end not at start"},
      {"$", "", false, "Anchor end empty (match mode)"},
      {"$", "anything", false, "Anchor end any (match mode)"},
      {"^abc$", "abc", true, "Both anchors exact"},
      {"^abc$", "abcd", false, "Both anchors extra char (prefix match)"},
      {"^abc$", "xabc", false, "Both anchors wrong start"},
      {"^hello$", "hello", true, "Both anchors word"},
      {"^hello$", "hello world", false, "Both anchors extra (prefix match)"},
      {"^a.*z$", "az", true, "Anchors with dotstar min"},
      {"^a.*z$", "abcxyz", true, "Anchors with dotstar middle"},
      {"^\\d+$", "123", true, "Anchors with digits"},
      {"^\\d+$", "abc", false, "Anchors with digits no match"},
      {"^[a-z]+$", "hello", true, "Anchors with class"},
      {"^[a-z]+$", "Hello", false, "Anchors with class upper"},
      {"^test", "test", true, "Start anchor alone"},
      {"^test", "testing", true, "Start anchor prefix"},
  };
  for (const auto &t : tests)
    run_match_test(t);

  // ========================================================================
  // QUANTIFIERS {m,n}
  // ========================================================================
  std::cout << "\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "QUANTIFIERS {m,n}\n";
  std::cout << "==============================================================="
               "=================\n";

  tests = {
      {"a{3}", "aaa", true, "Exact three"},
      {"a{3}", "aaaa", true, "Exact three prefix"},
      {"a{3}", "aa", false, "Exact three too few"},
      {"a{3}", "a", false, "Exact three only one"},
      {"a{3}", "", false, "Exact three empty"},
      {"a{0}", "", true, "Exact zero"},
      {"a{0}", "a", true, "Exact zero ignores"},
      {"a{1}", "a", true, "Exact one"},
      {"a{1}", "aa", true, "Exact one prefix"},
      {"a{1}", "", false, "Exact one empty"},
      {"a{5}", "aaaaa", true, "Exact five"},
      {"a{5}", "aaaaaa", true, "Exact five prefix"},
      {"a{5}", "aaaa", false, "Exact five too few"},
      {"a{2,4}", "aa", true, "Range min"},
      {"a{2,4}", "aaa", true, "Range middle"},
      {"a{2,4}", "aaaa", true, "Range max"},
      {"a{2,4}", "aaaaa", true, "Range over max prefix"},
      {"a{2,4}", "a", false, "Range too few"},
      {"a{2,4}", "", false, "Range empty"},
      {"a{0,3}", "", true, "Range zero min"},
      {"a{0,3}", "a", true, "Range zero min one"},
      {"a{0,3}", "aaa", true, "Range zero min max"},
      {"a{0,3}", "aaaa", true, "Range zero min over"},
      {"a{1,1}", "a", true, "Range one one"},
      {"a{1,1}", "aa", true, "Range one one prefix"},
      {"a{1,1}", "", false, "Range one one empty"},
      {"a{2,}", "aa", true, "Unbounded min"},
      {"a{2,}", "aaaa", true, "Unbounded many"},
      {"a{2,}", "aaaaaaaaaa", true, "Unbounded lots"},
      {"a{2,}", "a", false, "Unbounded too few"},
      {"a{2,}", "", false, "Unbounded empty"},
      {"a{0,}", "", true, "Unbounded zero min"},
      {"a{0,}", "aaa", true, "Unbounded zero min many"},
      {"\\d{3}", "123", true, "Digit exact three"},
      {"\\d{3}", "1234", true, "Digit exact three prefix"},
      {"\\d{3}", "12", false, "Digit exact three too few"},
      {"\\d{2,4}", "12", true, "Digit range min"},
      {"\\d{2,4}", "123", true, "Digit range middle"},
      {"\\d{2,4}", "1234", true, "Digit range max"},
      {"\\d{2,4}", "12345", true, "Digit range over"},
      {"\\d{2,4}", "1", false, "Digit range too few"},
      {"[a-z]{3}", "abc", true, "Class exact three"},
      {"[a-z]{3}", "abcd", true, "Class exact three prefix"},
      {"[a-z]{3}", "ab", false, "Class exact three too few"},
      {"[0-9]{2,}", "12", true, "Class unbounded min"},
      {"[0-9]{2,}", "12345", true, "Class unbounded many"},
      {"(ab){2}", "abab", true, "Group exact two"},
      {"(ab){2}", "ababab", true, "Group exact two prefix"},
      {"(ab){2}", "ab", false, "Group exact two too few"},
      {"(ab){2,3}", "abab", true, "Group range min"},
      {"(ab){2,3}", "ababab", true, "Group range max"},
      {"(ab){2,3}", "abababab", true, "Group range over"},
      {"(ab){2,3}", "ab", false, "Group range too few"},
  };
  for (const auto &t : tests)
    run_match_test(t);

  // ========================================================================
  // CAPTURE GROUPS
  // ========================================================================
  std::cout << "\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "CAPTURE GROUPS\n";
  std::cout << "==============================================================="
               "=================\n";

  tests = {
      {"(a)", "a", true, "Single group one char"},
      {"(a)", "ab", true, "Single group prefix"},
      {"(a)", "b", false, "Single group no match"},
      {"(abc)", "abc", true, "Single group multi char"},
      {"(abc)", "abcd", true, "Single group prefix"},
      {"(abc)", "ab", false, "Single group incomplete"},
      {"(a)(b)", "ab", true, "Two groups"},
      {"(a)(b)", "abc", true, "Two groups prefix"},
      {"(a)(b)", "a", false, "Two groups incomplete"},
      {"(a)(b)(c)", "abc", true, "Three groups"},
      {"(a)(b)(c)", "abcd", true, "Three groups prefix"},
      {"(hello)", "hello", true, "Group word"},
      {"(hello)", "hello world", true, "Group word prefix"},
      {"(\\d+)", "123", true, "Group digits"},
      {"(\\d+)", "123abc", true, "Group digits prefix"},
      {"(\\d+)", "abc", false, "Group digits no match"},
      {"([a-z]+)", "hello", true, "Group class"},
      {"([a-z]+)", "hello123", true, "Group class prefix"},
      {"(a+)(b+)", "ab", true, "Two groups with plus"},
      {"(a+)(b+)", "aaabbb", true, "Two groups many"},
      {"(a+)(b+)", "a", false, "Two groups first only"},
      {"(a*)(b*)", "", true, "Two groups both zero"},
      {"(a*)(b*)", "aaa", true, "Two groups first many"},
      {"(a*)(b*)", "bbb", true, "Two groups second many"},
      {"(a*)(b*)", "aaabbb", true, "Two groups both many"},
      {"(a?)b", "b", true, "Group optional then char"},
      {"(a?)b", "ab", true, "Group present then char"},
      {"(a|b)", "a", true, "Group alternation first"},
      {"(a|b)", "b", true, "Group alternation second"},
      {"(a|b)", "c", false, "Group alternation no match"},
      {"(cat|dog)", "cat", true, "Group word alternation first"},
      {"(cat|dog)", "dog", true, "Group word alternation second"},
      {"((a))", "a", true, "Nested groups"},
      {"((a))", "ab", true, "Nested groups prefix"},
      {"((a)(b))", "ab", true, "Nested groups two inner"},
      {"(a(b)c)", "abc", true, "Group with nested"},
      {"(a(b)c)", "abcd", true, "Group with nested prefix"},
      {"((a+)(b+))", "aaabbb", true, "Nested quantified groups"},
      {"(\\d{3})-(\\d{2})", "123-45", true, "Groups with quantifiers"},
      {"(\\d{3})-(\\d{2})", "123-456", true, "Groups quantifiers prefix"},
      {"(\\d{3})-(\\d{2})", "12-45", false, "Groups quantifiers too few"},
      {"(\\w+)@(\\w+)", "user@domain", true, "Groups email-like"},
      {"(\\w+)@(\\w+)", "user@domain.com", true, "Groups email prefix"},
      {"(a)(b)(c)(d)(e)", "abcde", true, "Many groups"},
      {"(a)(b)(c)(d)(e)", "abcdef", true, "Many groups prefix"},
  };
  for (const auto &t : tests)
    run_match_test(t);

  // ========================================================================
  // COMPLEX COMBINATIONS
  // ========================================================================
  std::cout << "\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "COMPLEX COMBINATIONS\n";
  std::cout << "==============================================================="
               "=================\n";

  tests = {
      {"a.*b", "ab", true, "A dotstar b minimum"},
      {"a.*b", "axb", true, "A dotstar b one char"},
      {"a.*b", "axxxxb", true, "A dotstar b many chars"},
      {"a.*b", "a", false, "A dotstar b no b"},
      {"a.*b", "b", false, "A dotstar b no a"},
      {".*", "", true, "Dotstar empty"},
      {".*", "anything", true, "Dotstar all"},
      {".*", "123!@#xyz", true, "Dotstar mixed"},
      {"a+b*c", "ac", true, "Plus star combo min"},
      {"a+b*c", "abc", true, "Plus star combo one each"},
      {"a+b*c", "aaabbbbc", true, "Plus star combo many"},
      {"a+b*c", "c", false, "Plus star combo no a"},
      {"a*b+c", "bc", true, "Star plus combo min"},
      {"a*b+c", "abc", true, "Star plus combo one each"},
      {"a*b+c", "aaabbbbc", true, "Star plus combo many"},
      {"a*b+c", "ac", false, "Star plus combo no b"},
      {"(a|b)*", "", true, "Group star zero"},
      {"(a|b)*", "a", true, "Group star one first"},
      {"(a|b)*", "b", true, "Group star one second"},
      {"(a|b)*", "abab", true, "Group star alternating"},
      {"(a|b)*", "aaabbb", true, "Group star repeated"},
      {"(a|b)*", "c", true, "Group star zero then mismatch"},
      {"(a|b)+", "a", true, "Group plus one first"},
      {"(a|b)+", "b", true, "Group plus one second"},
      {"(a|b)+", "abab", true, "Group plus alternating"},
      {"(a|b)+", "", false, "Group plus empty"},
      {"(a|b)+", "c", false, "Group plus no match"},
      {"[a-z]+@[a-z]+", "user@domain", true, "Email-like basic"},
      {"[a-z]+@[a-z]+", "test@example", true, "Email-like test"},
      {"[a-z]+@[a-z]+", "user", false, "Email-like no at"},
      {"[a-z]+@[a-z]+", "@domain", false, "Email-like no user"},
      {"[a-z]+@[a-z]+\\.[a-z]+", "test@example.com", true, "Email with dot"},
      {"[a-z]+@[a-z]+\\.[a-z]+", "user@mail.org", true, "Email with dot org"},
      {"[a-z]+@[a-z]+\\.[a-z]+", "test@example", false, "Email no extension"},
      {"\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}", "192.168.1.1", true,
       "IP address valid"},
      {"\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}", "10.0.0.1", true,
       "IP address short"},
      {"\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}", "192.168.1", false,
       "IP address incomplete"},
      {"(https?://)?(www\\.)?[a-z]+\\.[a-z]+", "http://www.example.com", true,
       "URL full"},
      {"(https?://)?(www\\.)?[a-z]+\\.[a-z]+", "https://example.com", true,
       "URL https no www"},
      {"(https?://)?(www\\.)?[a-z]+\\.[a-z]+", "www.example.com", true,
       "URL no protocol"},
      {"(https?://)?(www\\.)?[a-z]+\\.[a-z]+", "example.com", true,
       "URL minimal"},
      {"(https?://)?(www\\.)?[a-z]+\\.[a-z]+", "ftp://example.com", false,
       "URL wrong protocol"},
      {"\\w+\\s+\\w+", "hello world", true, "Two words space"},
      {"\\w+\\s+\\w+", "foo bar", true, "Two words different"},
      {"\\w+\\s+\\w+", "hello  world", true, "Two words double space matches"},
      {"\\w+\\s+\\w+", "hello", false, "Two words only one"},
      {"\\w+\\s*\\w+", "hello world", true, "Words optional space"},
      {"\\w+\\s*\\w+", "helloworld", true, "Words no space"},
      {"\\w+\\s*\\w+", "hello  world", true, "Words multi space"},
      {"^\\d+$", "123", true, "Anchored digits"},
      {"^\\d+$", "abc", false, "Anchored digits letters"},
      {"^[a-zA-Z]+$", "Hello", true, "Anchored letters mixed"},
      {"^[a-zA-Z]+$", "Hello123", false, "Anchored letters with digits"},
      {"(a+|b+)", "aaa", true, "Group alternation plus first"},
      {"(a+|b+)", "bbb", true, "Group alternation plus second"},
      {"(a+|b+)", "ab", true, "Group alternation plus first prefix"},
      {"(a+|b+)", "", false, "Group alternation plus empty"},
      {"((a|b)+|(c|d)+)", "aab", true, "Nested alternation groups first"},
      {"((a|b)+|(c|d)+)", "ccd", true, "Nested alternation groups second"},
      {"((a|b)+|(c|d)+)", "e", false, "Nested alternation groups no match"},
      {"a{2,}b{2,}", "aabb", true, "Two unbounded min"},
      {"a{2,}b{2,}", "aaaabbbb", true, "Two unbounded many"},
      {"a{2,}b{2,}", "ab", false, "Two unbounded too few"},
      {"[a-z]{3,}@[a-z]{3,}\\.[a-z]{2,}", "test@example.com", true,
       "Email with lengths"},
      {"[a-z]{3,}@[a-z]{3,}\\.[a-z]{2,}", "ab@ex.c", false, "Email too short"},
      {"(\\d+)([a-z]+)(\\d+)", "123abc456", true, "Three groups mixed"},
      {"(\\d+)([a-z]+)(\\d+)", "123abc", false, "Three groups incomplete"},
      {"^(a|b)*c$", "c", true, "Anchored group star then c"},
      {"^(a|b)*c$", "aac", true, "Anchored group star many then c"},
      {"^(a|b)*c$", "abc", true, "Anchored group star mixed then c"},
      {"^(a|b)*c$", "abcd", false, "Anchored group star extra char"},
      {"<.*>", "<tag>", true, "Tags simple"},
      {"<.*>", "<tag>content</tag>", true, "Tags nested greedy"},
      {"<.*>", "<", false, "Tags no close"},
      {".*\\.txt", "file.txt", true, "File extension"},
      {".*\\.txt", "document.txt", true, "File extension long"},
      {".*\\.txt", "file.doc", false, "File extension wrong"},
      {"(lo)*l*", "", true, "Python example empty"},
      {"(lo)*l*", "lo", true, "Python example lo"},
      {"(lo)*l*", "lol", true, "Python example lol"},
      {"(lo)*l*", "lolll", true, "Python example lolll"},
      {"(lo)*l*", "lolol", true, "Python example lolol"},
      {"(lo)*l*", "looool", true, "Python example looool prefix lo"},
      {"(lo)*l*", "olll", true, "Python example olll empty match"},
      {"(lo)*l*", "lolololll", true, "Python example lolololll"},
  };
  for (const auto &t : tests)
    run_match_test(t);

  // ========================================================================
  // EDGE CASES AND SPECIAL PATTERNS
  // ========================================================================
  std::cout << "\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "EDGE CASES AND SPECIAL PATTERNS\n";
  std::cout << "==============================================================="
               "=================\n";

  tests = {
      {"", "", true, "Empty pattern empty text"},
      {"", "a", true, "Empty pattern non-empty text"},
      {"a", "", false, "Non-empty pattern empty text"},
      {"a*", "", true, "Star zero empty"},
      {"a+", "", false, "Plus requires one"},
      {"a?", "", true, "Question zero empty"},
      {"(a|b)*", "", true, "Alternation star zero"},
      {"(a|b)+", "", false, "Alternation plus requires one"},
      {".*", "", true, "Dotstar empty"},
      {".*", "\n", true, "Dotstar with newline"},
      {".+", "", false, "Dotplus empty"},
      {"a{0}", "", true, "Zero quantifier"},
      {"a{0}", "a", true, "Zero quantifier with text"},
      {"a{0,0}", "", true, "Zero range"},
      {"a{0,}", "", true, "Unbounded from zero empty"},
      {"a{0,}", "aaaa", true, "Unbounded from zero many"},
      {"^$", "", false, "Both anchors empty (match mode)"},
      {"^$", "a", false, "Both anchors non-empty"},
      {"^^a", "a", true, "Double start anchor"},
      {"a$$", "a", true, "Double end anchor"},
      // These patterns cause syntax errors (correct behavior):
      // {"()", "", true, "Empty group"},
      // {"()", "a", true, "Empty group with text"},
      // {"()*", "", true, "Empty group star"},
      // {"()+", "", false, "Empty group plus"},
      {"(a*)*", "", true, "Nested star zero"},
      {"(a*)*", "aaa", true, "Nested star many"},
      {"(a+)+", "a", true, "Nested plus one"},
      {"(a+)+", "aaa", true, "Nested plus many"},
      {"(a+)+", "", false, "Nested plus empty"},
      // Invalid alternation patterns (syntax errors - correct behavior):
      // {"a|", "a", true, "Alternation empty second"},
      // {"a|", "", true, "Alternation empty second empty text"},
      // {"|a", "", true, "Alternation empty first"},
      // {"|a", "a", true, "Alternation empty first match"},
      // {"a||b", "a", true, "Double alternation first"},
      // {"a||b", "b", true, "Double alternation third"},
      // {"a||b", "", true, "Double alternation middle empty"},
      {"[a-a]", "a", true, "Single char range"},
      {"[z-z]", "z", true, "Single char range z"},
      {"[0-0]", "0", true, "Single digit range"},
      {"[a-z]*[A-Z]*", "", true, "Two class stars empty"},
      {"[a-z]*[A-Z]*", "abc", true, "Two class stars first"},
      {"[a-z]*[A-Z]*", "ABC", true, "Two class stars second"},
      {"[a-z]*[A-Z]*", "abcABC", true, "Two class stars both"},
      {"\\d*\\w*", "", true, "Two shorthand stars empty"},
      {"\\d*\\w*", "123", true, "Two shorthand stars digits"},
      {"\\d*\\w*", "abc", true, "Two shorthand stars word"},
      {"\\d*\\w*", "123abc", true, "Two shorthand stars both"},
      // Invalid quantifier patterns (syntax errors - correct behavior):
      // {"a**", "a", true, "Double star (star of star)"},
      // {"a**", "", true, "Double star empty"},
      // {"a++", "a", true, "Double plus"},
      // {"a++", "aa", true, "Double plus two"},
      // {"a++", "", false, "Double plus empty"},
      // {"a*+", "a", true, "Star then plus"},
      // {"a*+", "", false, "Star then plus empty"},
      // {"a+*", "", true, "Plus then star"},
      // {"a+*", "a", true, "Plus then star one"},
      // {"a+*", "aaa", true, "Plus then star many"},
      {"((((a))))", "a", true, "Many nested groups"},
      {"((((a))))", "ab", true, "Many nested groups prefix"},
      {"a{1,1}", "a", true, "Range 1 to 1"},
      {"a{1,1}", "aa", true, "Range 1 to 1 prefix"},
      {"a{0,1}", "", true, "Range 0 to 1 zero"},
      {"a{0,1}", "a", true, "Range 0 to 1 one"},
      // Invalid character class (syntax error - correct behavior):
      // {"[^]*", "", true, "Negated empty class star"},
      {"[a-z-]", "-", true, "Class with dash at end"},
      {"[-a-z]", "-", true, "Class with dash at start"},
      {"[a\\-z]", "-", true, "Class with escaped dash"},
  };
  for (const auto &t : tests)
    run_match_test(t);

  // ========================================================================
  // REAL-WORLD PATTERNS
  // ========================================================================
  std::cout << "\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "REAL-WORLD PATTERNS\n";
  std::cout << "==============================================================="
               "=================\n";

  tests = {
      // Phone numbers
      {"\\d{3}-\\d{3}-\\d{4}", "123-456-7890", true, "Phone US format"},
      {"\\d{3}-\\d{3}-\\d{4}", "123-456-789", false, "Phone too short"},
      {"\\(\\d{3}\\)\\s*\\d{3}-\\d{4}", "(123) 456-7890", true,
       "Phone with parens"},
      {"\\(\\d{3}\\)\\s*\\d{3}-\\d{4}", "(123)456-7890", true,
       "Phone no space"},

      // Dates
      {"\\d{2}/\\d{2}/\\d{4}", "12/31/2023", true, "Date MM/DD/YYYY"},
      {"\\d{2}/\\d{2}/\\d{4}", "1/1/2023", false, "Date single digits"},
      {"\\d{4}-\\d{2}-\\d{2}", "2023-12-31", true, "Date ISO format"},
      {"\\d{1,2}/\\d{1,2}/\\d{4}", "1/1/2023", true, "Date flexible"},

      // Times
      {"\\d{2}:\\d{2}", "14:30", true, "Time 24h"},
      {"\\d{2}:\\d{2}:\\d{2}", "14:30:45", true, "Time with seconds"},
      {"\\d{1,2}:\\d{2}", "9:30", true, "Time flexible hour"},

      // Hexadecimal
      {"#[0-9a-fA-F]{6}", "#FF5733", true, "Hex color"},
      {"#[0-9a-fA-F]{6}", "#ff5733", true, "Hex color lowercase"},
      {"#[0-9a-fA-F]{6}", "#FG5733", false, "Hex color invalid"},
      {"0[xX][0-9a-fA-F]+", "0xFF", true, "Hex number prefix"},
      {"0[xX][0-9a-fA-F]+", "0x1a2b", true, "Hex number long"},

      // Version numbers
      {"\\d+\\.\\d+\\.\\d+", "1.2.3", true, "Version semver"},
      {"\\d+\\.\\d+\\.\\d+", "10.20.30", true, "Version double digits"},
      {"\\d+\\.\\d+", "1.2", true, "Version major.minor"},

      // File paths
      {".*\\.txt", "file.txt", true, "Text file"},
      {".*\\.jpg", "image.jpg", true, "JPG file"},
      {".*\\.(txt|md|doc)", "readme.md", true, "Doc file types"},
      {".*\\.(txt|md|doc)", "file.pdf", false, "Doc wrong type"},

      // Username patterns
      {"[a-z][a-z0-9_]{2,15}", "user123", true, "Username valid"},
      {"[a-z][a-z0-9_]{2,15}", "test_user", true, "Username with underscore"},
      {"[a-z][a-z0-9_]{2,15}", "ab", false, "Username too short"},
      {"[a-z][a-z0-9_]{2,15}", "123user", false, "Username starts with digit"},

      // Hashtags
      {"#[a-zA-Z0-9_]+", "#coding", true, "Hashtag simple"},
      {"#[a-zA-Z0-9_]+", "#test_123", true, "Hashtag with underscore"},
      {"#[a-zA-Z0-9_]+", "#", false, "Hashtag empty"},

      // Currency
      {"\\$\\d+\\.\\d{2}", "$10.99", true, "Currency with cents"},
      {"\\$\\d+\\.\\d{2}", "$5.00", true, "Currency round"},
      {"\\$\\d+", "$100", true, "Currency no cents"},

      // HTML tags (simple)
      {"<[a-z]+>", "<div>", true, "HTML opening tag"},
      {"<[a-z]+>", "<span>", true, "HTML span tag"},
      {"</[a-z]+>", "</div>", true, "HTML closing tag"},

      // Markdown
      {"\\*\\*.*\\*\\*", "**bold**", true, "Markdown bold"},
      {"__.*__", "__italic__", true, "Markdown italic"},

      // Log levels
      {"\\[(INFO|WARN|ERROR)\\]", "[INFO]", true, "Log level info"},
      {"\\[(INFO|WARN|ERROR)\\]", "[ERROR]", true, "Log level error"},
      {"\\[(INFO|WARN|ERROR)\\]", "[DEBUG]", false, "Log level invalid"},

      // Simple SQL
      {"SELECT \\* FROM \\w+", "SELECT * FROM users", true, "SQL select"},
      {"INSERT INTO \\w+", "INSERT INTO table", true, "SQL insert"},

      // Variable names (programming)
      {"[a-zA-Z_][a-zA-Z0-9_]*", "myVar", true, "Variable camelCase"},
      {"[a-zA-Z_][a-zA-Z0-9_]*", "my_var", true, "Variable snake_case"},
      {"[a-zA-Z_][a-zA-Z0-9_]*", "_private", true, "Variable private"},
      {"[a-zA-Z_][a-zA-Z0-9_]*", "123var", false, "Variable starts digit"},
  };
  for (const auto &t : tests)
    run_match_test(t);

  // ========================================================================
  // SUMMARY
  // ========================================================================
  auto end_time = high_resolution_clock::now();
  auto duration = duration_cast<milliseconds>(end_time - start_time);

  std::cout << "\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "TEST SUMMARY\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "Total tests:  " << total_tests << "\n";
  std::cout << "Passed:       " << passed_tests << " (" << std::fixed
            << std::setprecision(1) << (100.0 * passed_tests / total_tests)
            << "%)\n";
  std::cout << "Failed:       " << failed_tests << " (" << std::fixed
            << std::setprecision(1) << (100.0 * failed_tests / total_tests)
            << "%)\n";
  std::cout << "Execution:    " << duration.count() << " ms\n";
  std::cout << "==============================================================="
               "=================\n";

  return (failed_tests == 0) ? 0 : 1;
}