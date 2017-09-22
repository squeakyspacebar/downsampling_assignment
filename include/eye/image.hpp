#ifndef EYE_IMAGE_HPP
#define EYE_IMAGE_HPP

#include <vector>
#include <eye/common.hpp>

namespace eye {
    /**
     * Convenience wrapper for multiarrays.
     */
    class Image {
        public:

        image_array_t img_array;
        std::size_t num_dims;
        std::vector<std::size_t> shape;

        Image(image_array_t img_array);
    };
}
#endif
