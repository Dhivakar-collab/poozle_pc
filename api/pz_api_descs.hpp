#ifndef PZ_API_DESCS_HPP
#define PZ_API_DESCS_HPP

#include <pz_util.hpp>
// other necessary headers

#define PZ_API /* API functions for Poozle library */

/*
API functions packed inside Poozle namespace

Example :

pzl is name of our namespace. Hence a C++ user will use our
library as pzl::poozle_function(args...)

namespace pzl {
    PZ_API void pz_input_as_file(std::string file);
    // other API functions ....
};

*/

#endif // PZ_API_DESCS_HPP