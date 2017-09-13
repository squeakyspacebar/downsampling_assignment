#include <chrono>
#include <iostream>
#include <string>
#include <eye/constants.hpp>
#include <eye/functions.hpp>
#include <eye/math.hpp>
#include <eye/image.hpp>
#include <eye/utility.hpp>

const std::size_t IMAGE_DIMS = 2;
using namespace std::chrono;

int main() {
    high_resolution_clock::time_point start = high_resolution_clock::now();

    std::string timestamp = eye::get_timestamp();

    // Generate an image with randomized dimensions and values.
    eye::Image img(eye::generate_randomized_image(IMAGE_DIMS));

    high_resolution_clock::time_point img_t1 = high_resolution_clock::now();

    // Find the power of 2 of the smallest dimension of the image.
    std::size_t min_l = eye::find_min_l(img);

    // For each power of 2 from 1 to min_l, calculate the downsampled image and
    // write it to file.
    for (std::size_t i = 1; i <= min_l; i++) {
        high_resolution_clock::time_point t1 = high_resolution_clock::now();

        std::cout << "Current downsampling level (" << i << ")" << std::endl;
        eye::Image ds_img(eye::process_image(img, i));

        std::string filename = timestamp + "_ds_" + std::to_string(IMAGE_DIMS) +
            "d_img_l" + std::to_string(i) + ".csv";
        eye::write_to_file(ds_img, filename);

        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(t2 - t1).count();
        std::cout << "ELAPSED TIME FOR RUN: " << duration << "ms" << std::endl;
    }

    high_resolution_clock::time_point img_t2 = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(img_t2 - img_t1).count();
    std::cout << "ELAPSED TIME FOR IMAGE: " << duration << "ms" << std::endl;

    high_resolution_clock::time_point stop = high_resolution_clock::now();
    auto elapsed = duration_cast<milliseconds>(stop - start).count();
    std::cout << "TOTAL ELAPSED TIME: " << elapsed << "ms" << std::endl;

    return 0;
}
