#include <chrono>
#include <iostream>
#include <string>
#include <eye/functions.hpp>
#include <eye/image.hpp>
#include <eye/utility.hpp>

const std::size_t IMAGE_DIMS = 2;
using namespace std::chrono;

int main() {
    high_resolution_clock::time_point start = high_resolution_clock::now();

    std::string timestamp = eye::get_timestamp();

    // Generate an image with randomized dimensions and values.
    eye::Image img(eye::generate_randomized_image(IMAGE_DIMS));

    // Create container to hold images.
    std::vector<eye::Image> ds_images;

    high_resolution_clock::time_point img_t1 = high_resolution_clock::now();

    // Downsample images and retrieve them.
    ds_images = eye::process_image(img);

    high_resolution_clock::time_point img_t2 = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(img_t2 - img_t1).count();
    std::cout << "ELAPSED TIME TO PROCESS IMAGE: " << duration << "ms" << std::endl;

    high_resolution_clock::time_point stop = high_resolution_clock::now();
    auto elapsed = duration_cast<milliseconds>(stop - start).count();
    std::cout << "TOTAL ELAPSED TIME: " << elapsed << "ms" << std::endl;

    return 0;
}
