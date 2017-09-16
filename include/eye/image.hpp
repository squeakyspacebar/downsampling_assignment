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

        image_array img_array;
        std::size_t num_dims;
        std::vector<std::size_t> shape;

        Image(image_array img_array);
    };

    inline Image::Image(image_array img_array) {
        this->img_array = img_array;
        this->num_dims = this->img_array.dimension();
        this->shape = std::vector<std::size_t>(this->num_dims);
        for (std::size_t i = 0; i < this->num_dims; i++) {
            this->shape[i] = img_array.shape(i);
        }
    }
}
#endif
