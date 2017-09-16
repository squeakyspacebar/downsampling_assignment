#ifndef EYE_FUNCTIONS_HPP
#define EYE_FUNCTIONS_HPP

#include <string>
#include <vector>
#include <eye/common.hpp>
#include <eye/image.hpp>

namespace eye {
    std::vector<eye::Image> process_image(const Image & img);
    void write_to_file(const Image & img, const std::string & filename);
    Image generate_randomized_image(const std::size_t dims);
    void fill_image(Image & img);
    std::size_t find_min_l(const Image & img);
    Image downsample_image(const Image & img, const std::size_t l);
    Image create_reduced_image(const Image & img, const std::size_t dim_size);
    image_data_t find_mode(const Image & img, const std::size_t dim_size,
        const std::size_t start_index);
}
#endif
