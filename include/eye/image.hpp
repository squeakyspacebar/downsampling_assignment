#ifndef EYE_IMAGE_HPP
#define EYE_IMAGE_HPP

#include <vector>
#include <andres/marray.hxx>

typedef andres::Marray<int> image_array;

namespace eye {
    /**
     * Convenience wrapper for multiarrays.
     */
    class Image {
        public:

        image_array img_array;
        std::size_t num_dims;
        std::vector<std::size_t> shape;

        Image(image_array img_array);
    };

}
#endif
