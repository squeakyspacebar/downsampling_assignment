#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <andres/marray.hxx>
#include <eye/constants.hpp>
#include <eye/functions.hpp>
#include <eye/math.hpp>
#include <eye/image.hpp>
#include <eye/utility.hpp>

const std::size_t IMAGE_DIMS = 3;
typedef andres::Marray<int> image_array;
typedef std::shared_ptr<eye::Image> img_ptr;
using namespace std::chrono;

int main() {
    high_resolution_clock::time_point start = high_resolution_clock::now();

    std::string timestamp = eye::get_timestamp();

    img_ptr img = eye::generate_randomized_image(IMAGE_DIMS);

    std::size_t min_l = eye::find_min_l(img);

    for (std::size_t i = 1; i <= min_l; i++) {
        std::cout << "Current downsampling level (" << i << ")" << std::endl;
        img_ptr ds_img = eye::process_image(img, i);

        std::string filename = timestamp + "_ds_" + std::to_string(IMAGE_DIMS) +
            "d_img_l" + std::to_string(i) + ".csv";
        eye::write_to_file(ds_img, filename);
    }

    high_resolution_clock::time_point stop = high_resolution_clock::now();

    auto elapsed = duration_cast<milliseconds>(stop - start).count();

    std::cout << "TOTAL ELAPSED TIME: " << elapsed << "ms" << std::endl;

    return 0;
}
