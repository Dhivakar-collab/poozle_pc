#ifndef PZ_API_DESCS_HPP
#define PZ_API_DESCS_HPP

#include <pz_util.hpp>
// other necessary headers

#define PZ_API /* API functions for Poozle library */

namespace pzl {
/*
    Example :
    PZ_API void pz_input_as_file(std::string file);
    // other API functions ....
*/

namespace pzstd = ::PzStd;
namespace pzerror = ::PzError;
}; // namespace pzl

#endif // PZ_API_DESCS_HPP