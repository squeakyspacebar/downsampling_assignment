#ifndef EYE_IMAGE_HPP
#define EYE_IMAGE_HPP

#include <mutex>
#include <vector>
#include <andres/marray.hxx>

typedef andres::Marray<int> image_array;

namespace eye {
    /**
     * Multidimensional array container to make it easier to pass associated
     * calculations around.
     */
    class Image {
        public:

        image_array img_array;
        std::size_t num_dims;
        std::vector<std::size_t> shape;

        Image(image_array & img_array) {
            this->img_array = img_array;
            this->num_dims = this->img_array.dimension();

            // Store shape in a more convenient format for passing.
            this->shape = std::vector<std::size_t>(this->num_dims);
            for (std::size_t i = 0; i < this->num_dims; i++) {
                this->shape[i] = this->img_array.shape(i);
            }
        }

        private:

    };
}
#endif
