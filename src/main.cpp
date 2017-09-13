#include <iostream>
#include <memory>
#include <string>
#include <andres/marray.hxx>
#include <eye/constants.hpp>
#include <eye/functions.hpp>
#include <eye/math.hpp>
#include <eye/image.hpp>
#include <eye/utility.hpp>

typedef andres::Marray<int> image_array;

int main() {
    std::string timestamp = eye::get_timestamp();

    std::shared_ptr<eye::Image> img = eye::generate_randomized_image(eye::DIM);

    std::size_t min_l = eye::find_min_l(img);

    for (std::size_t i = 1; i <= min_l; i++) {
        std::cout << "Current downsampling level (" << i << ")" << std::endl;
        std::shared_ptr<eye::Image> ds_img = eye::process_image(img, i);

        std::string filename = timestamp + "_ds_" + std::to_string(eye::DIM) +
            "d_img_l" + std::to_string(i) + ".csv";
        eye::write_to_file(ds_img, filename);
    }

    return 0;
}
