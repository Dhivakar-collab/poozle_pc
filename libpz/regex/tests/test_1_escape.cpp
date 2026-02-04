#include "NfaMatcher.hpp"
#include "nfa.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace std::chrono;

struct EscapeTestCase {
  std::string input;
  std::string expected_output;
  std::string description;
};

int total_tests = 0;
int passed_tests = 0;
int failed_tests = 0;

// Helper function to call escape (creates a dummy matcher since escape is an
// instance method)
std::string escape_helper(const std::string &text) {
  State dummy_state(StateType::MATCH);
  NfaMatcher dummy_matcher(&dummy_state);
  return dummy_matcher.escape(text);
}

void run_escape_test(const EscapeTestCase &test) {
  total_tests++;
  std::string result = escape_helper(test.input);
  bool success = (result == test.expected_output);

  if (success) {
    passed_tests++;
    std::cout << "[SUCCESS] ";
  } else {
    failed_tests++;
    std::cout << "[FAILURE] ";
  }

  std::cout << std::setw(40) << std::left << test.description
            << " | Input: " << std::setw(30) << std::left << test.input
            << " | Expected: " << std::setw(30) << std::left
            << test.expected_output << " | Got: " << result << "\n";
}

int main() {
  auto start_time = high_resolution_clock::now();

  std::vector<EscapeTestCase> escape_tests;

  std::cout << "\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "ESCAPE FUNCTION TESTS (Similar to Python's re.escape())\n";
  std::cout << "==============================================================="
               "=================\n";
  std::cout << "\n";

  escape_tests = {
      // Basic metacharacters - operators
      {".", "\\.", "Escape dot"},
      {"*", "\\*", "Escape star"},
      {"+", "\\+", "Escape plus"},
      {"?", "\\?", "Escape question"},
      {"|", "\\|", "Escape pipe"},

      // Anchors
      {"^", "\\^", "Escape caret"},
      {"$", "\\$", "Escape dollar"},

      // Escape character itself
      {"\\", "\\\\", "Escape backslash"},

      // Grouping characters
      {"(", "\\(", "Escape left paren"},
      {")", "\\)", "Escape right paren"},
      {"[", "\\[", "Escape left bracket"},
      {"]", "\\]", "Escape right bracket"},
      {"{", "\\{", "Escape left brace"},
      {"}", "\\}", "Escape right brace"},

      // Character class special
      {"-", "\\-", "Escape dash"},

      // No special characters - should remain unchanged
      {"abc", "abc", "Plain text - no escape needed"},
      {"hello", "hello", "Plain text - hello"},
      {"123", "123", "Numbers - no escape needed"},
      {"_test", "_test", "Underscore - no escape needed"},
      {"", "", "Empty string"},
      {"test_123", "test_123", "Alphanumeric with underscore"},
      {"Hello World", "Hello World", "Text with space"},

      // Multiple metacharacters
      {"a.b", "a\\.b", "Escape in middle"},
      {".*", "\\.\\*", "Escape dot star"},
      {"a+b", "a\\+b", "Escape plus in middle"},
      {"(abc)", "\\(abc\\)", "Escape parens"},
      {"[a-z]", "\\[a\\-z\\]", "Escape char class"},
      {"{1,3}", "\\{1,3\\}", "Escape braces"},
      {"^start$", "\\^start\\$", "Escape anchors"},
      {"a|b|c", "a\\|b\\|c", "Escape pipes"},

      // Complex patterns
      {"(a+|b*)?", "\\(a\\+\\|b\\*\\)\\?", "Escape complex pattern"},
      {"[0-9]+", "\\[0\\-9\\]\\+", "Escape digit pattern"},
      {".*\\.txt$", "\\.\\*\\\\\\.txt\\$", "Escape file pattern"},
      {"\\d{2,4}", "\\\\d\\{2,4\\}", "Escape quantifier"},
      {"^[A-Z].*[a-z]$", "\\^\\[A\\-Z\\]\\.\\*\\[a\\-z\\]\\$",
       "Escape full pattern"},

      // Real-world examples
      {"192.168.1.1", "192\\.168\\.1\\.1", "Escape IP address"},
      {"file.txt", "file\\.txt", "Escape filename"},
      {"$100", "\\$100", "Escape price"},
      {"(555) 123-4567", "\\(555\\) 123\\-4567", "Escape phone"},
      {"user@domain.com", "user@domain\\.com", "Escape email"},
      {"C:\\Users\\test", "C:\\\\Users\\\\test", "Escape Windows path"},
      {"a[0]", "a\\[0\\]", "Escape array index"},
      {"key=value", "key=value", "No special chars - key=value"},
      {"*.*", "\\*\\.\\*", "Escape wildcards"},
      {"a**b", "a\\*\\*b", "Escape double star"},

      // Edge cases - repeated metacharacters
      {".....", "\\.\\.\\.\\.\\.", "Multiple dots"},
      {"((()))", "\\(\\(\\(\\)\\)\\)", "Nested parens"},
      {"|||", "\\|\\|\\|", "Multiple pipes"},
      {"[[[", "\\[\\[\\[", "Multiple brackets"},
      {"***", "\\*\\*\\*", "Multiple stars"},
      {"+++", "\\+\\+\\+", "Multiple plus"},
      {"???", "\\?\\?\\?", "Multiple questions"},
      {"$$$", "\\$\\$\\$", "Multiple dollars"},
      {"^^^", "\\^\\^\\^", "Multiple carets"},
      {"---", "\\-\\-\\-", "Multiple dashes"},

      // Mixed special and normal characters
      {"test.py", "test\\.py", "Python file"},
      {"README.md", "README\\.md", "Markdown file"},
      {"2+2=4", "2\\+2=4", "Math expression"},
      {"100% certain", "100% certain", "Percentage"},
      {"cost: $50", "cost: \\$50", "Cost with dollar"},
      {"option (a)", "option \\(a\\)", "Option with parens"},
      {"range [1-10]", "range \\[1\\-10\\]", "Range with brackets"},

      // Regex patterns themselves
      {"\\w+", "\\\\w\\+", "Escape word pattern"},
      {"\\d+", "\\\\d\\+", "Escape digit pattern"},
      {"\\s*", "\\\\s\\*", "Escape space pattern"},
      {"[a-zA-Z0-9]", "\\[a\\-zA\\-Z0\\-9\\]", "Escape alphanum class"},

      // URL-like strings
      {"http://example.com", "http://example\\.com", "HTTP URL"},
      {"https://test.org/path?query=1", "https://test\\.org/path\\?query=1",
       "HTTPS URL with query"},
      {"ftp://server.net", "ftp://server\\.net", "FTP URL"},

      // Code-like patterns
      {"if (x > 0)", "if \\(x > 0\\)", "If statement"},
      {"arr[i++]", "arr\\[i\\+\\+\\]", "Array increment"},
      {"func(a, b)", "func\\(a, b\\)", "Function call"},
      {"x * y + z", "x \\* y \\+ z", "Math expression"},

      // Special combinations
      {".*?", "\\.\\*\\?", "Non-greedy wildcard"},
      {".+?", "\\.\\+\\?", "Non-greedy plus"},
      {"((?:abc)+)", "\\(\\(\\?:abc\\)\\+\\)", "Non-capturing group"},
      {"\\b\\w+\\b", "\\\\b\\\\w\\+\\\\b", "Word boundary pattern"},

      // Corner cases
      {" ", " ", "Single space"},
      {"  ", "  ", "Double space"},
      {"\t", "\t", "Tab character"},
      {"\n", "\n", "Newline character"},
      {"a\tb\nc", "a\tb\nc", "Tab and newline"},

      // Very long strings with metacharacters
      {".........", "\\.\\.\\.\\.\\.\\.\\.\\.\\.", "Many dots"},
      {"(((((", "\\(\\(\\(\\(\\(", "Many opening parens"},
      {")))))", "\\)\\)\\)\\)\\)", "Many closing parens"},

      // Mixed everything
      {"^(test|demo)[0-9]+$", "\\^\\(test\\|demo\\)\\[0\\-9\\]\\+\\$",
       "Complex regex pattern"},
      {"*.{txt,pdf}", "\\*\\.\\{txt,pdf\\}", "File glob pattern"},
      {"user@host.com:8080", "user@host\\.com:8080", "Host with port"},

      // ========================================================================
      // DEADLY COMPLEX TESTS - Stress Testing
      // ========================================================================

      // All metacharacters in one string
      {".|*+?()[]{}^$\\-", "\\.\\|\\*\\+\\?\\(\\)\\[\\]\\{\\}\\^\\$\\\\\\-",
       "All metacharacters together"},

      // Nested special characters
      {"((([[{{**++??}}]])))",
       "\\(\\(\\(\\[\\[\\{\\{\\*\\*\\+\\+\\?\\?\\}\\}\\]\\]\\)\\)\\)",
       "Deeply nested specials"},

      // Alternating metacharacters and text
      {".a*b+c?d|e^f$g(h)i[j]k{l}m\\n-o",
       "\\.a\\*b\\+c\\?d\\|e\\^f\\$g\\(h\\)i\\[j\\]k\\{l\\}m\\\\n\\-o",
       "Alternating meta and text"},

      // Long sequences of same metacharacter
      {".....................",
       "\\.\\.\\.\\.\\.\\.\\.\\.\\.\\.\\.\\.\\.\\.\\.\\.\\.\\.\\.\\.\\.",
       "21 dots"},
      {"********************",
       "\\*\\*\\*\\*\\*\\*\\*\\*\\*\\*\\*\\*\\*\\*\\*\\*\\*\\*\\*\\*",
       "20 stars"},
      {"++++++++++++++++++++",
       "\\+\\+\\+\\+\\+\\+\\+\\+\\+\\+\\+\\+\\+\\+\\+\\+\\+\\+\\+\\+",
       "20 pluses"},
      {"||||||||||||||||||||",
       "\\|\\|\\|\\|\\|\\|\\|\\|\\|\\|\\|\\|\\|\\|\\|\\|\\|\\|\\|\\|",
       "20 pipes"},
      {"((((((((((((((((((((",
       "\\(\\(\\(\\(\\(\\(\\(\\(\\(\\(\\(\\(\\(\\(\\(\\(\\(\\(\\(\\(",
       "20 left parens"},
      {"))))))))))))))))))",
       "\\)\\)\\)\\)\\)\\)\\)\\)\\)\\)\\)\\)\\)\\)\\)\\)\\)\\)",
       "18 right parens"},

      // Complex real-world patterns
      {"(?:(?:[0-9]{1,3}\\.){3}[0-9]{1,3})",
       "\\(\\?:\\(\\?:\\[0\\-9\\]\\{1,3\\}\\\\\\.\\)\\{3\\}\\[0\\-9\\]\\{1,3\\}"
       "\\)",
       "IPv4 non-capturing regex"},
      {"^(?=.*[A-Z])(?=.*[a-z])(?=.*[0-9])(?=.*[!@#$%^&*]).{8,}$",
       "\\^\\(\\?=\\.\\*\\[A\\-Z\\]\\)\\(\\?=\\.\\*\\[a\\-z\\]\\)\\(\\?=\\.\\*"
       "\\[0\\-9\\]\\)\\(\\?=\\.\\*\\[!@#\\$%\\^&\\*\\]\\)\\.\\{8,\\}\\$",
       "Password strength regex"},
      {"([a-zA-Z0-9._%-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,})",
       "\\(\\[a\\-zA\\-Z0\\-9\\._%\\-\\]\\+@\\[a\\-zA\\-Z0\\-9\\.\\-\\]\\+"
       "\\\\\\.\\[a\\-zA\\-Z\\]\\{2,\\}\\)",
       "Email validation regex"},
      {"\\b(https?|ftp)://[^\\s/$.?#].[^\\s]*\\b",
       "\\\\b\\(https\\?\\|ftp\\)://\\[\\^\\\\s/"
       "\\$\\.\\?#\\]\\.\\[\\^\\\\s\\]\\*\\\\b",
       "URL matching regex"},

      // SQL injection patterns (to escape for safe searching)
      {"'; DROP TABLE users; --", "'; DROP TABLE users; \\-\\-",
       "SQL injection 1"},
      {"1' OR '1'='1", "1' OR '1'='1", "SQL injection 2"},
      {"admin'--", "admin'\\-\\-", "SQL injection 3"},

      // Shell command injection patterns
      {"; rm -rf /", "; rm \\-rf /", "Shell injection 1"},
      {"| cat /etc/passwd", "\\| cat /etc/passwd", "Shell injection 2"},
      {"&& echo 'hacked'", "&& echo 'hacked'", "Shell injection 3"},
      {"`whoami`", "`whoami`", "Backtick injection"},
      {"$(command)", "\\$\\(command\\)", "Command substitution"},

      // Path traversal patterns
      {"../../etc/passwd", "\\.\\./\\.\\./etc/passwd", "Path traversal 1"},
      {"..\\..\\windows\\system32", "\\.\\.\\\\\\.\\.\\\\windows\\\\system32",
       "Path traversal Windows"},
      {"%2e%2e%2f%2e%2e%2f", "%2e%2e%2f%2e%2e%2f", "URL encoded traversal"},

      // XSS patterns
      {"<script>alert('XSS')</script>", "<script>alert\\('XSS'\\)</script>",
       "XSS basic"},
      {"javascript:alert(1)", "javascript:alert\\(1\\)", "XSS javascript"},
      {"<img src=x onerror=alert(1)>", "<img src=x onerror=alert\\(1\\)>",
       "XSS img tag"},

      // Regex bombs (catastrophic backtracking)
      {"(a+)+b", "\\(a\\+\\)\\+b", "Regex bomb 1"},
      {"(a*)*b", "\\(a\\*\\)\\*b", "Regex bomb 2"},
      {"(a|a)*b", "\\(a\\|a\\)\\*b", "Regex bomb 3"},
      {"(a|ab)*c", "\\(a\\|ab\\)\\*c", "Regex bomb 4"},

      // Unicode and special characters (ASCII only for now)
      {"\\x00\\x01\\x02", "\\\\x00\\\\x01\\\\x02", "Hex escape sequences"},
      {"\\u0000\\u0001", "\\\\u0000\\\\u0001", "Unicode escape sequences"},
      {"\\n\\r\\t\\f\\v", "\\\\n\\\\r\\\\t\\\\f\\\\v", "Control char escapes"},

      // Complex nested groups
      {"((a)(b))((c)(d))", "\\(\\(a\\)\\(b\\)\\)\\(\\(c\\)\\(d\\)\\)",
       "Nested groups 1"},
      {"(((((a)))))", "\\(\\(\\(\\(\\(a\\)\\)\\)\\)\\)", "5-level nesting"},
      {"((a|b)|(c|d))", "\\(\\(a\\|b\\)\\|\\(c\\|d\\)\\)",
       "Nested alternations"},

      // Lookaheads and lookbehinds
      {"(?=abc)", "\\(\\?=abc\\)", "Positive lookahead"},
      {"(?!abc)", "\\(\\?!abc\\)", "Negative lookahead"},
      {"(?<=abc)", "\\(\\?<=abc\\)", "Positive lookbehind"},
      {"(?<!abc)", "\\(\\?<!abc\\)", "Negative lookbehind"},

      // Quantifier variations
      {"{0}", "\\{0\\}", "Zero quantifier"},
      {"{1}", "\\{1\\}", "One quantifier"},
      {"{999}", "\\{999\\}", "Large quantifier"},
      {"{1,}", "\\{1,\\}", "Open-ended quantifier"},
      {"{,10}", "\\{,10\\}", "Upper bound only"},
      {"{5,5}", "\\{5,5\\}", "Exact same bounds"},
      {"{0,0}", "\\{0,0\\}", "Zero-zero quantifier"},

      // Character class edge cases
      {"[^]", "\\[\\^\\]", "Negated empty class"},
      {"[]]", "\\[\\]\\]", "Class with closing bracket"},
      {"[[]", "\\[\\[\\]", "Class with opening bracket"},
      {"[\\]]", "\\[\\\\\\]\\]", "Escaped bracket in class"},
      {"[a-zA-Z0-9_.-]", "\\[a\\-zA\\-Z0\\-9_\\.\\-\\]", "Complex char class"},
      {"[^a-zA-Z0-9]", "\\[\\^a\\-zA\\-Z0\\-9\\]", "Negated alphanum"},

      // Boundary assertions
      {"\\b\\w+\\b", "\\\\b\\\\w\\+\\\\b", "Word boundaries"},
      {"\\B\\w+\\B", "\\\\B\\\\w\\+\\\\B", "Non-word boundaries"},
      {"^$", "\\^\\$", "Empty line match"},
      {"^.*$", "\\^\\.\\*\\$", "Full line match"},

      // Backreferences
      {"(a)\\1", "\\(a\\)\\\\1", "Backreference 1"},
      {"(\\w+)\\s+\\1", "\\(\\\\w\\+\\)\\\\s\\+\\\\1",
       "Backreference with space"},
      {"(['\"]).*?\\1", "\\(\\['\"\\]\\)\\.\\*\\?\\\\1", "Quote matching"},

      // Mixed quotation marks (quotes are NOT regex metacharacters, so not
      // escaped)
      {"\"test\"", "\"test\"", "Double quotes"},
      {"'test'", "'test'", "Single quotes"},
      {"`test`", "`test`", "Backticks"},
      {"\"'`test`'\"", "\"'`test`'\"", "All quotes mixed"},

      // File paths with various separators
      {"/usr/local/bin", "/usr/local/bin", "Unix path"},
      {"C:\\Program Files\\App", "C:\\\\Program Files\\\\App",
       "Windows path with space"},
      {"\\\\server\\share\\file", "\\\\\\\\server\\\\share\\\\file",
       "UNC path"},
      {"./relative/path", "\\./relative/path", "Relative path with dot"},
      {"../parent/path", "\\.\\./parent/path", "Parent path"},

      // Data URIs and special URLs
      {"data:text/plain;base64,SGVsbG8=", "data:text/plain;base64,SGVsbG8=",
       "Data URI"},
      {"file:///C:/path/file.txt", "file:///C:/path/file\\.txt", "File URI"},
      {"mailto:user@example.com?subject=Test",
       "mailto:user@example\\.com\\?subject=Test", "Mailto URI"},

      // Mathematical expressions
      {"(x+y)*(a-b)/c^2", "\\(x\\+y\\)\\*\\(a\\-b\\)/c\\^2", "Math expression"},
      {"f(x)=x^2+2*x+1", "f\\(x\\)=x\\^2\\+2\\*x\\+1", "Function notation"},
      {"∑(i=1 to n) i^2", "∑\\(i=1 to n\\) i\\^2", "Summation (with unicode)"},

      // Log patterns
      {"[2024-01-28 14:30:45.123] ERROR: Failed",
       "\\[2024\\-01\\-28 14:30:45\\.123\\] ERROR: Failed",
       "Log with timestamp"},
      {"192.168.1.1 - - [28/Jan/2024:14:30:45 +0000]",
       "192\\.168\\.1\\.1 \\- \\- \\[28/Jan/2024:14:30:45 \\+0000\\]",
       "Apache log format"},

      // JSON strings (quotes are not regex metacharacters)
      {"{\"key\": \"value\", \"num\": 123}",
       "\\{\"key\": \"value\", \"num\": 123\\}", "JSON object"},
      {"[1, 2, 3, \"test\"]", "\\[1, 2, 3, \"test\"\\]", "JSON array"},

      // Regex named groups
      {"(?<name>pattern)", "\\(\\?<name>pattern\\)", "Named group"},
      {"(?P<word>\\w+)", "\\(\\?P<word>\\\\w\\+\\)", "Python named group"},

      // Conditional patterns
      {"(?(1)yes|no)", "\\(\\?\\(1\\)yes\\|no\\)", "Conditional pattern"},

      // Very long mixed pattern
      {"^(?=.*[A-Z])(?=.*[a-z])(?=.*[0-9])(?=.*[!@#$%^&*()_+-={}|:\"<>?,.]).{"
       "12,}$",
       "\\^\\(\\?=\\.\\*\\[A\\-Z\\]\\)\\(\\?=\\.\\*\\[a\\-z\\]\\)\\(\\?=\\.\\*"
       "\\[0\\-9\\]\\)\\(\\?=\\.\\*\\[!@#\\$%\\^&\\*\\(\\)_\\+\\-=\\{\\}\\|:\"<"
       ">\\?,\\.\\]\\)\\.\\{12,\\}\\$",
       "Strong password regex"},

      // Credit card validation regex
      {"^(?:4[0-9]{12}(?:[0-9]{3})?|5[1-5][0-9]{14}|3[47][0-9]{13})$",
       "\\^\\(\\?:4\\[0\\-9\\]\\{12\\}\\(\\?:\\[0\\-9\\]\\{3\\}\\)\\?\\|5\\["
       "1\\-5\\]\\[0\\-9\\]\\{14\\}\\|3\\[47\\]\\[0\\-9\\]\\{13\\}\\)\\$",
       "Credit card regex"},

      // IPv6 pattern
      {"([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}",
       "\\(\\[0\\-9a\\-fA\\-F\\]\\{1,4\\}:\\)\\{7\\}\\[0\\-9a\\-fA\\-F\\]\\{1,"
       "4\\}",
       "IPv6 regex"},

      // Extremely complex nested pattern
      {"((a*)*|(b+)+)*c", "\\(\\(a\\*\\)\\*\\|\\(b\\+\\)\\+\\)\\*c",
       "Evil nested quantifiers"},

      // All ASCII printable special chars (only regex metacharacters get
      // escaped)
      {"!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~",
       "!\"#\\$%&'\\(\\)\\*\\+,\\-\\./:;<=>\\?@\\[\\\\\\]\\^_`\\{\\|\\}~",
       "All special ASCII chars"},

      // Extreme length metacharacter sequences
      {std::string(50, '.'),
       std::string(100, '\\') +
           std::string(50, '.').replace(0, 50, std::string(50, '.').c_str()),
       "50 dots (extreme)"},
      {std::string(100, '*'),
       std::string(200, '\\') +
           std::string(100, '*').replace(0, 100, std::string(100, '*').c_str()),
       "100 stars (extreme)"},
  };

  // Generate the correct escaped strings for the extreme length tests
  escape_tests[escape_tests.size() - 2].expected_output = "";
  for (int i = 0; i < 50; i++)
    escape_tests[escape_tests.size() - 2].expected_output += "\\.";

  escape_tests[escape_tests.size() - 1].expected_output = "";
  for (int i = 0; i < 100; i++)
    escape_tests[escape_tests.size() - 1].expected_output += "\\*";

  for (const auto &t : escape_tests) {
    run_escape_test(t);
  }

  // Summary
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
