#ifndef EYE_FUNCTIONS_HPP
#define EYE_FUNCTIONS_HPP

#include <memory>
#include <string>
#include <eye/image.hpp>

namespace eye {
    Image generate_randomized_image(const std::size_t dims);
    void fill_image(Image & img);
    std::size_t find_min_l(const Image img);
    Image process_image(const Image img, const std::size_t l);
    Image create_reduced_image(const Image img, const std::size_t dim_size);
    int find_mode(const Image img, const std::size_t dim_size,
        const std::size_t start_index);
    void write_result_to_image(Image & img, const int index, const int mode);
    void write_to_file(const Image img, const std::string & filename);
}
#endif
