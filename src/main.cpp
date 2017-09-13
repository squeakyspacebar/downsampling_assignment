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

    std::shared_ptr<eye::Image> img = eye::generate_randomized_image();

    std::size_t min_l = eye::find_min_l(img);
    std::cout << "Minimum l: " << min_l << std::endl;

    for (std::size_t l = 1; l <= min_l; l++) {
        std::cout << "Current downsampling level (" << l << ")" << std::endl;
        std::shared_ptr<eye::Image> ds_img = eye::process_image(img, l);

        std::string filename = timestamp + "_ds_" + std::to_string(eye::DIM) +
            "d_img_l" + std::to_string(l) + ".csv";
        eye::write_to_file(ds_img, filename);
    }

    return 0;
}
