#include <cmath>
#include <iostream>
#include <map>
#include <random>
#include <vector>
#include <andres/marray.hxx>

void generate_random_image() {
    std::cout << "generate_random_image() entered" << std::endl;

    // Initialize random number generation.
    std::random_device rd;
    std::minstd_rand gen(rd());

    // Generate randomized dimensionality for the image.
    std::uniform_int_distribution<int> dim_dist(1, 4);
    const int n_dims = dim_dist(gen);
    std::cout << "Generated dimensionality: " << n_dims << std::endl;

    // Generate size of each dimension, which is 2 to the power of the number
    // generated.
    size_t * shape = new size_t[n_dims];
    std::uniform_int_distribution<int> dim_size_dist(1, 10);
    for (int i = 0; i < n_dims; i++) {
        int dim_size = std::pow(2, dim_size_dist(gen));
        shape[i] = dim_size;
        std::cout << "Dim(" << i + 1 << "): " << shape[i] << std::endl;
    }

    // Create image array according to the generated shape.
    andres::Marray<int> img(shape, shape + n_dims, 0);
    delete shape;

    // Bookkeeping for determining mode of generated image.
    std::map<int, int> mode_map;
    mode_map.insert(std::pair<int, int>(-1, -1));

    // Generates random values for each element of the image.
    std::uniform_int_distribution<int> ran_dist(0, 2);
    for (size_t i = 0; i < img.size(); i++) {
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
    for (const auto &kv : mode_map) {
        if (kv.first < 0) {
            continue;
        }
        std::cout << kv.first << ": " << kv.second << " instance(s)" << std::endl;
    }
}

int main() {
    generate_random_image();
    std::cout << "generate_random_image() exited" << std::endl;

    return 0;
}
