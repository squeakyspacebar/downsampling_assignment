#ifndef EYE_FUNCTIONS_HPP
#define EYE_FUNCTIONS_HPP

#include <string>
#include <vector>
#include <eye/common.hpp>
#include <eye/image.hpp>

namespace eye {
    std::vector<Image> process_image(const Image & img);
    void write_to_file(const Image & img, const std::string & filename);
    Image generate_randomized_image(const std::size_t dims);
    void fill_image(Image & img);
    std::size_t find_max_l(const Image & img);
    std::vector<Image> downsample_image(const Image & img,
        const std::size_t l,
        const std::size_t max_l);
    Image downsample_reduce(const mode_array_t & prev_mode_array,
        const Image & img,
        const std::size_t l,
        const std::size_t max_l,
        std::vector<Image> & ds_images);
    Image create_reduced_image(const Image & img, const std::size_t dim_size);
    mode_pair_t find_mode(const Image & img,
        const std::size_t start_index);
    mode_pair_t reduce_modes(const mode_array_t & prev_mode_array,
        const std::size_t start_index);
}
#endif
