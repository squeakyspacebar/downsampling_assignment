#ifndef EYE_FUNCTIONS_HPP
#define EYE_FUNCTIONS_HPP

#include <memory>
#include <string>
#include <andres/marray.hxx>
#include <eye/image.hpp>

typedef andres::Marray<int> image_array;

namespace eye {
    std::shared_ptr<Image> generate_randomized_image(const std::size_t dims);
    void fill_image_array(const std::shared_ptr<Image> img);
    std::size_t find_min_l(const std::shared_ptr<Image> img);
    std::shared_ptr<Image> process_image(const std::shared_ptr<Image> img,
        const std::size_t l);
    std::shared_ptr<Image> create_reduced_image_array(
        const std::shared_ptr<Image> img,
        const std::size_t dim_size);
    int find_mode(const std::shared_ptr<Image> img, const std::size_t l,
        const std::size_t start_index);
    void write_result_to_image(const std::shared_ptr<Image> img,
        const int index,
        const int mode);
    void write_to_file(const std::shared_ptr<Image> img,
        const std::string & filename);
}
#endif
