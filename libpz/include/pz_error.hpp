#ifndef PZ_ERROR_HPP
#define PZ_ERROR_HPP
#include <pz_cxx_std.hpp>

namespace PzError {
enum class PzErrorType;
// other error / warning related functions or enums
inline void report_error(PzErrorType type, const std::string &message) {
  throw std::runtime_error("PzError " + message + " (Code: " +
                           std::to_string(static_cast<int>(type)) + ")");
}
}; // namespace PzError

enum class PzError::PzErrorType {
  PZ_NO_ERROR = 0,
  PZ_WRONG_ARGS,
  PZ_ERROR,
  PZ_INVALID_INPUT,
  PZ_BUFFER_ACCESS_FAILED,
  PZ_ANALYSIS_FAILED,
  PZ_INVALID_ANALYSIS_TYPE,
  PZ_FILE_NOT_FOUND,
};

namespace fuzzy {

class FileNotFoundError : public std::runtime_error {
public:
    explicit FileNotFoundError(std::string_view path) 
    : std::runtime_error("Could not open file " + std::string(path)) {}
};

class EmptyDataError : public std::runtime_error {
public:
    EmptyDataError() : std::runtime_error("no data found in input file") {}
};

class InvalidQueryError : public std::runtime_error {
public:
    explicit InvalidQueryError(std::string_view reason) 
    : std::runtime_error("Invalid Query " + std::string(reason)) {}
};

class LongPatternError : public std::runtime_error {
public:
    LongPatternError() : std::runtime_error("Pattern length is more than 63") {}
};
}

#endif // PZ_ERROR_HPP