#include <cstdint>
#include <cmath>
#include <future>
#include <iostream>
#include <map>
#include <thread>
#include <queue>
#include <andres/marray.hxx>
#include <zi/concurrency/concurrency.hpp>

const unsigned int MAX_THREADS = std::thread::hardware_concurrency() + 1;
const int DIM = 3;
typedef andres::Marray<int> img_array;
typedef std::packaged_task<int(img_array, int, int)> fm_task;

class FindMode {
    FindMode() {}

    int operator() (const img_array & img, const int dim_size, const int starting_index) {
        std::cout << "find_mode() entered" << std::endl;

        //const size_t * shape = img.shape();
        int positions[DIM] = { 0 };
        int place = 0;
        int index = starting_index;
        int max = dim_size;
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
                if (place == DIM - 1) {
                    overflow = true;
                }

                positions[place] = 0;
                // Carry the one.
                place++;
                positions[place]++;
            }

            // Calculate the proper index.
            index = positions[0];
            for (int i = 1; i < DIM; i++) {
                index += positions[i] * img.shape(i - 1);
            }

            place = 0;
        }

        // Output diagnostic information.
        std::cout << "mode of view is " << mode_map[-1] << " with " <<
            mode_map[mode_map[-1]] << " instances" << std::endl;
        // Output each number encountered with its frequency.
        for (const auto &kv : mode_map) {
            if (kv.first < 0) {
                continue;
            }
            std::cout << kv.first << ": " << kv.second << " instance(s)" << std::endl;
        }

        std::cout << "find_mode() exited" << std::endl;

        return mode_map[-1];
    }
};

img_array generate_image() {
    std::cout << "generate_image() entered" << std::endl;

    // Initialize random number generation.
    std::random_device rd;
    std::minstd_rand gen(rd());

    // Generate randomized dimensionality for the image.
    std::uniform_int_distribution<std::size_t> dim_size_dist(1, 10);
    std::size_t * shape = new std::size_t[DIM];
    for (int i = 0; i < DIM; i++) {
        std::size_t dim_size = dim_size_dist(gen);
        shape[i] = std::pow(2, dim_size);
        std::cout << "Dim(" << i + 1 << "): " << shape[i] << std::endl;
    }

    // Initialize blank image.
    img_array img(shape, shape + DIM);
    delete shape;

    // Bookkeeping for determining mode of generated image.
    std::map<int, int> mode_map;
    mode_map.insert(std::pair<int, int>(-1, -1));

    // Generates random values for each element of the image.
    std::uniform_int_distribution<int> ran_dist(0, 2);
    std::size_t img_size = img.size();
    for (std::size_t i = 0; i < img_size; i++) {
        int key = ran_dist(gen);
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
    for (const auto &kv : mode_map) {
        if (kv.first < 0) {
            continue;
        }
        std::cout << kv.first << ": " << kv.second << " instance(s)" << std::endl;
    }

    std::cout << "generate_image() exited" << std::endl;

    return img;
}

void process_image(const img_array & img, int l) {
    std::cout << "process_image() entered" << std::endl;

    int dim_size = std::pow(2, l);
    std::cout << "dim_size: " << dim_size << std::endl;

    // Determine the starting index for each processing window.
    std::vector<int> starting_indices;

    //const size_t * shape = img.shape();
    std::size_t positions[DIM] = { 0 };
    int place = 0;
    int index = 0;
    int stride = dim_size;
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
        for (int i = 1; i < DIM; i++) {
            index += positions[i] * img.shape(i - 1);
        }

        place = 0;
    }

    // Calculate shape of downsampled image.
    std::cout << "Reduced shape for l(" << l << "):" << std::endl;
    std::vector<int> reduced_dims;
    for (int i = 0; i < DIM; i++) {
        int reduced_dim_size = img.shape(i) / dim_size;
        if (reduced_dim_size > 1) {
            reduced_dims.push_back(reduced_dim_size);
        }
    }

    const std::size_t num_reduced_dims = reduced_dims.size();
    std::size_t * reduced_shape = new std::size_t[num_reduced_dims];
    for (std::size_t i = 0; i < num_reduced_dims; i++) {
        reduced_shape[i] = reduced_dims[i];
    }
    img_array reduced_img(reduced_shape, reduced_shape + num_reduced_dims);

    for (std::size_t i = 0; i < num_reduced_dims; i++) {
        std::cout << "Dim(" << reduced_img.shape(i) << ")" << std::endl;
    }

    /*std::queue<fm_task> tasks;
    std::vector<std::future<int>> futures;

    for (auto & i : starting_indices) {
        std::cout << i << std::endl;
        fm_task task(findmode);
        tasks.push(task);
        futures.push_back(task.get_future());
    }

    for (auto & task : tasks) {
        std::thread(std::move(task), )
    }*/

    std::cout << "process_image() exited" << std::endl;
}

size_t log2 (size_t v) {
    if (v == 0) return SIZE_MAX;
    if (v == 1) return 0;

    size_t l = 0;
    while (v > 1) {
        v >>= 1;
        l++;
    }

    return l;
}

size_t find_min_l(const img_array & img) {
    std::size_t min_l = SIZE_MAX;

    std::size_t num_dims = img.dimension();
    for (std::size_t i = 0; i < num_dims; i++) {
        size_t dim = img.shape(i);
        if (dim < min_l) {
            min_l = log2(dim);
        }
    }

    return min_l;
}

int main() {
    std::cout << "MAX_THREADS(" << MAX_THREADS << ")" << std::endl;

    img_array img = generate_image();

    std::size_t min_l = find_min_l(img);
    std::cout << "Minimum l: " << min_l << std::endl;

    for (std::size_t l = 1; l <= min_l; l++) {
        process_image(img, l);
    }

    return 0;
}
