#ifndef EYE_FUNCTIONS_HPP
#define EYE_FUNCTIONS_HPP

#include <string>
#include <vector>
#include <eye/common.hpp>
#include <eye/image.hpp>

namespace eye {
    typedef std::pair<Image, std::vector<mode_map_t>> image_pair_t;

    std::vector<Image> process_image(const Image & img);
    void write_to_file(const Image & img, const std::string & filename);
    Image generate_randomized_image(const std::size_t dims);
    void fill_image(Image & img);
    std::size_t find_max_l(const Image & img);
    image_pair_t downsample_image(const Image & img);
    image_pair_t downsample_reduce(const image_pair_t & img_pair);
    Image create_reduced_image(const Image & img, const std::size_t dim_size);
    mode_pair_t find_mode(const Image & img, const std::size_t start_index);
    mode_pair_t reduce_modes(const Image & img,
        const mode_array_t & mode_array,
        const std::size_t start_index);
}
#endif
