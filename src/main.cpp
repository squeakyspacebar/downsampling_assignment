#include <cstdint>
#include <future>
#include <iostream>
#include <map>
#include <random>
#include <thread>
#include <queue>
#include <andres/marray.hxx>
#include <boost/thread/future.hpp>
#include <eye/thread_pool.hpp>
#include <eye/utility.hpp>

const unsigned int MAX_WORK_THREADS = std::thread::hardware_concurrency();
const int DIM = 3;
typedef andres::Marray<int> img_array;

img_array generate_image();
img_array process_image(const img_array & img, const std::size_t l);
int find_mode(const img_array & img, const std::size_t l,
    const std::size_t starting_index);
std::size_t find_min_l(const img_array & img);

int find_mode(
        const img_array & img,
        const std::size_t l,
        const std::size_t starting_index) {

    std::size_t num_dims = img.dimension();
    std::vector<size_t> positions(num_dims);
    std::size_t place = 0;
    std::size_t index = starting_index;
    std::size_t max = eye::pow(2, l);
    bool overflow = false;

    // Bookkeeping for determining mode of generated image.
    std::map<int, int> mode_map;
    mode_map.insert(std::pair<int, int>(-1, -1));

    while (!overflow) {
        int key = img(index);

        // Keep a count of the values encountered to determine mode.
        if (mode_map.count(key) > 0) {
            mode_map[key]++;
        } else {
            mode_map.insert(std::pair<int, int>(key, 1));
        }

        // Update which key represents the mode.
        if (mode_map[key] > mode_map[mode_map[-1]]) {
            mode_map[-1] = key;
        }

        // Update positional counter.
        positions[0]++;
        while (positions[place] == max) {
            if (place == num_dims - 1) {
                overflow = true;
                break;
            }

            // Carry the one.
            positions[place] = 0;
            place++;
            positions[place]++;
        }

        // Calculate the proper index.
        index = positions[0];
        for (std::size_t i = 1; i < num_dims; i++) {
            index += positions[i] * img.shape(i - 1);
        }

        place = 0;
    }

    return mode_map[-1];
}

img_array generate_image() {
    std::cout << "generate_image() entered" << std::endl;

    // Initialize random number generation.
    std::random_device device;
    std::minstd_rand generator(device());

    // Generate randomized dimensionality for the image.
    std::uniform_int_distribution<int> dim_size_dist(1, 8);
    std::size_t * shape = new std::size_t[DIM];
    for (std::size_t i = 0; i < DIM; i++) {
        std::size_t dim_size = dim_size_dist(generator);
        shape[i] = eye::pow(2, dim_size);
        std::cout << "Dim(" << i + 1 << "): " << shape[i] << std::endl;
    }

    // Initialize blank image.
    img_array img(shape, shape + DIM);
    delete [] shape;

    // Bookkeeping for determining mode of generated image.
    std::map<int, int> mode_map;
    mode_map.insert(std::pair<int, int>(-1, -1));

    // Generates random values for each element of the image.
    std::uniform_int_distribution<int> val_dist(0, 2);
    std::size_t img_size = img.size();
    for (std::size_t i = 0; i < img_size; i++) {
        int key = val_dist(generator);
        img(i) = key;

        // Keeps a count of the values encountered to determine mode.
        if (mode_map.count(key) > 0) {
            mode_map[key]++;
        } else {
            mode_map.insert(std::pair<int, int>(key, 1));
        }

        // Update which key represents the mode.
        if (mode_map[key] > mode_map[mode_map[-1]]) {
            mode_map[-1] = key;
        }
    }

    // Output diagnostic information.
    std::cout << "Mode of generated image is " << mode_map[-1] << " with " <<
        mode_map[mode_map[-1]] << " instances out of " << img.size() << std::endl;
    // Output each number encountered with its frequency.
    for (const auto & kv : mode_map) {
        if (kv.first < 0) {
            continue;
        }
        std::cout << kv.first << ": " << kv.second << " instance(s)" << std::endl;
    }

    std::cout << "generate_image() exited" << std::endl;

    return img;
}

img_array process_image(const img_array & img, const std::size_t l) {
    std::cout << "process_image() entered" << std::endl;

    std::size_t num_dims = img.dimension();
    std::size_t dim_size = eye::pow(2, l);
    std::cout << "dim_size: " << dim_size << std::endl;

    // Determine the starting index for each processing window.
    std::vector<std::size_t> starting_indices;

    std::vector<size_t> positions(num_dims, 0);
    std::size_t place = 0;
    std::size_t index = 0;
    std::size_t stride = dim_size;
    bool overflow = false;

    std::cout << "Stride: " << stride << std::endl;
    while (!overflow) {
        starting_indices.push_back(index);

        // Update positional counter.
        positions[0] += stride;
        while (positions[place] >= img.shape(place)) {
            if (place >= (DIM - 1)) {
                overflow = true;
                break;
            }

            positions[place] = 0;
            // Carry the one.
            place++;
            positions[place] += stride;
        }

        // Calculate the proper index.
        index = positions[0];
        for (std::size_t i = 1; i < num_dims; i++) {
            index += positions[i] * img.shape(i - 1);
        }

        place = 0;
    }

    // Calculate shape of downsampled image.
    std::cout << "Reduced shape for l(" << l << "):" << std::endl;
    std::vector<std::size_t> reduced_dims;
    for (std::size_t i = 0; i < num_dims; i++) {
        std::size_t reduced_dim_size = img.shape(i) / dim_size;
        if (reduced_dim_size > 1) {
            reduced_dims.push_back(reduced_dim_size);
        }
    }

    std::size_t num_reduced_dims = reduced_dims.size();
    std::size_t * reduced_shape = new size_t[num_reduced_dims];
    for (std::size_t i = 0; i < num_reduced_dims; i++) {
        reduced_shape[i] = reduced_dims[i];
    }
    img_array reduced_img(reduced_shape, reduced_shape + num_reduced_dims);
    delete [] reduced_shape;

    for (std::size_t i = 0; i < num_reduced_dims; i++) {
        std::cout << "Dim(" << reduced_img.shape(i) << ")" << std::endl;
    }

    std::cout << "Number of tasks: " << starting_indices.size() << std::endl;

    std::vector<boost::unique_future<int>> modes;

    // Initialize task manager.
    eye::ThreadPool tm(MAX_WORK_THREADS);
    for (auto & i : starting_indices) {
        modes.push_back(tm.add_task(find_mode, boost::ref(img), l, i));
    }
    tm.stop();

    for (auto & f : modes) {
        std::cout << "Future result: " << f.get() << std::endl;
    }

    std::cout << "process_image() exited" << std::endl;

    return reduced_img;
}

std::size_t find_min_l(const img_array & img) {
    std::size_t min_l = SIZE_MAX;

    std::size_t num_dims = img.dimension();
    for (std::size_t i = 0; i < num_dims; i++) {
        std::size_t dim = eye::log2(img.shape(i));
        if (dim < min_l) {
            min_l = dim;
        }
    }

    return min_l;
}

int main() {
    std::cout << "MAX_WORK_THREADS(" << MAX_WORK_THREADS << ")" << std::endl;

    img_array img = generate_image();

    std::size_t min_l = find_min_l(img);
    std::cout << "Minimum l: " << min_l << std::endl;

    process_image(img, min_l);

    return 0;
}
