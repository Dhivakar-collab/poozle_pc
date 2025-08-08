#ifndef PZ_ERROR_HPP
#define PZ_ERROR_HPP

namespace PzError {
enum class PzErrorType;
// other error / warning related functions or enums
}; // namespace PzError

enum class PzError::PzErrorType {
  PZ_NO_ERROR = 0,
  PZ_WRONG_ARGS,
  PZ_ERROR,
};

#endif PZ_ERROR_HPP