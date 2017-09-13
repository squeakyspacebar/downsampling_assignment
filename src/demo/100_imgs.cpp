#include <chrono>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <andres/marray.hxx>
#include <eye/constants.hpp>
#include <eye/functions.hpp>
#include <eye/math.hpp>
#include <eye/image.hpp>
#include <eye/utility.hpp>

const std::size_t IMAGE_DIMS = 2;
typedef andres::Marray<int> image_array;
using namespace std::chrono;

int main() {
    high_resolution_clock::time_point start = high_resolution_clock::now();

    std::string timestamp = eye::get_timestamp();

    // Generate test image sizes systematically.
    std::vector<std::vector<std::size_t>> img_shapes;
    auto f = [&](const std::vector<std::size_t> positions,
        const std::size_t index) {
        std::vector<std::size_t> img_shape(IMAGE_DIMS);
        for (std::size_t i = 0; i < IMAGE_DIMS; i++) {
            img_shape[i] = eye::pow(2, positions[i] + 1);
        }
        img_shapes.push_back(std::move(img_shape));
    };

    std::vector<std::size_t> dim_shape(IMAGE_DIMS, 8);
    eye::polytopic_loop(dim_shape, f);

    std::size_t num_images = img_shapes.size();
    std::cout << num_images << " images generated" << std::endl;

    for (std::size_t i = 0; i < num_images; i++) {
        // Grab image shape.
        std::cout << "Current image dimensions [";
        std::size_t shape[IMAGE_DIMS];
        for (std::size_t j = 0; j < IMAGE_DIMS; j++) {
            shape[j] = img_shapes[i][j];
            std::cout << shape[j];
            if (j < (IMAGE_DIMS - 1)) {
                std::cout << ", ";
            }
        }
        std::cout << "]" << std::endl;

        // Create an image array and fill it with random values.
        image_array img_array(shape, shape + IMAGE_DIMS);
        eye::Image img(img_array);
        eye::fill_image(img);

        // Find the power of 2 of the smallest dimension of the image.
        std::size_t min_l = eye::find_min_l(img);

        // For each power of 2 from 1 to min_l, calculate the downsampled image and
        // write it to file.
        for (std::size_t i = 1; i <= min_l; i++) {
            std::cout << "Current downsampling level (" << i << ")" << std::endl;
            eye::Image ds_img(eye::process_image(img, i));

            std::string filename = timestamp + "_ds_" + std::to_string(IMAGE_DIMS) +
                "d_img_l" + std::to_string(i) + ".csv";
            eye::write_to_file(ds_img, filename);
        }
    }

    high_resolution_clock::time_point stop = high_resolution_clock::now();
    auto elapsed = duration_cast<milliseconds>(stop - start).count();
    std::cout << "TOTAL ELAPSED TIME: " << elapsed << "ms" << std::endl;

    return 0;
}
