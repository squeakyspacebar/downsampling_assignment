#include <chrono>
#include <iostream>
#include <string>
#include <eye/common.hpp>
#include <eye/functions.hpp>
#include <eye/image.hpp>
#include <eye/utility.hpp>

const std::size_t IMAGE_DIMS = 2;
using namespace std::chrono;

int main() {
    high_resolution_clock::time_point start = high_resolution_clock::now();

    // Generate a square image with randomized values.
    std::size_t shape[] = { 1024, 1024 };
    image_array img_array(shape, shape + IMAGE_DIMS);
    eye::Image img(img_array);
    eye::fill_image(img);

    // Create container to hold images.
    std::vector<eye::Image> ds_images;

    high_resolution_clock::time_point img_t1 = high_resolution_clock::now();

    // Downsample images and retrieve them.
    ds_images = eye::process_image(img);

    high_resolution_clock::time_point img_t2 = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(img_t2 - img_t1).count();
    std::cout << "ELAPSED TIME TO PROCESS IMAGE: " << duration << "ms" << std::endl;

    // Write original image to file.
    std::string filename = "1024_ds_" + std::to_string(IMAGE_DIMS) +
        "d_img_orig.csv";
    eye::write_to_file(img, filename);

    // Write downsampled images to file.
    std::size_t num_images = ds_images.size();
    for (std::size_t i = 0; i < num_images; i++) {
        std::string filename = "1024_ds_" + std::to_string(IMAGE_DIMS) +
            "d_img_l" + std::to_string(i + 1) + ".csv";
        eye::write_to_file(ds_images[i], filename);
    }

    high_resolution_clock::time_point stop = high_resolution_clock::now();
    auto elapsed = duration_cast<milliseconds>(stop - start).count();
    std::cout << "TOTAL ELAPSED TIME: " << elapsed << "ms" << std::endl;

    return 0;
}
