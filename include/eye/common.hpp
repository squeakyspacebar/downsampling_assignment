#ifndef EYE_COMMON_HPP
#define EYE_COMMON_HPP

#include <map>
#include <utility>
#include <andres/marray.hxx>

namespace eye {
    typedef unsigned int image_data_t;
    typedef std::map<image_data_t, std::size_t> mode_map_t;
    typedef std::pair<mode_map_t, image_data_t> mode_pair_t;
    typedef andres::Marray<unsigned int> image_array_t;
    typedef std::vector<mode_map_t> mode_array_t;
}
#endif
