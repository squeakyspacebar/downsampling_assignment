#include <algorithm>
#include <chrono>
#include <iostream>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <eye/common.hpp>
#include <eye/constants.hpp>
#include <eye/functions.hpp>
#include <eye/image.hpp>
#include <eye/math.hpp>
#include <eye/thread_pool.hpp>
#include <eye/utility.hpp>

namespace eye {
    /**
     * Takes an image and computes a series of downsampled images.
     */
    std::vector<Image> process_image(const Image & img) {
        // Find the power of 2 of the smallest dimension of the image.
        std::size_t max_l = find_max_l(img);

        // Initial count of modes.
        std::vector<Image> ds_images;
        image_pair_t img_pair = downsample_image(img);
        ds_images.push_back(img_pair.first);

        // Reduce modes to produce each successive level of downsampling.
        for (std::size_t l = 2; l < max_l; l++) {
            img_pair = downsample_reduce(img_pair);
            ds_images.push_back(img_pair.first);
        }

        return ds_images;
    }

    void write_to_file(const Image & img, const std::string & filename) {
        std::ofstream outfile;
        outfile.open(filename);

        outfile << "[";
        for (std::size_t i = 0; i < img.num_dims; i++) {
            outfile << img.shape[i];
            if (i < (img.num_dims - 1)) {
                outfile << ",";
            }
        }
        outfile << "]" << std::endl;

        // Define logic to be run inside polytopic loop.
        auto f = [&](const std::vector<std::size_t> & positions,
            const std::size_t & index) -> void {
            // Print value position.
            outfile << "<";
            for (std::size_t i = 0; i < img.num_dims; i++) {
                outfile << positions[i];
                if (i < (img.num_dims - 1)) {
                    outfile << ",";
                }
            }
            outfile << ">,";
            // Print value at position.
            outfile << img.img_array(index) << std::endl;
        };

        // Write to file.
        polytopic_loop(img.shape, f);

        outfile.close();
        std::cout << "Wrote output to " << filename << std::endl;
    }

    Image generate_randomized_image(const std::size_t dims) {
        // Initialize random number generation.
        long int seed = std::chrono::high_resolution_clock::now()
            .time_since_epoch().count();
        std::minstd_rand generator(seed);

        // Generate random dimensionality for the image.
        std::uniform_int_distribution<int> pow_dist(1, 8);
        std::size_t * shape = new std::size_t[dims];
        std::cout << "Generated image dimensions: [";
        for (std::size_t i = 0; i < dims; i++) {
            std::size_t dim_size = 2 << pow_dist(generator);
            shape[i] = dim_size;
            std::cout << shape[i];
            if (i < (dims - 1)) {
                std::cout << ",";
            }
        }
        std::cout << "]" << std::endl;

        Image img(image_array_t(shape, shape + dims));
        delete [] shape;
        fill_image(img);

        return img;
    }

    /**
     * Fills an image with random values.
     */
    void fill_image(Image & img) {
        // Initialize random number generation.
        long int seed = std::chrono::high_resolution_clock::now()
            .time_since_epoch().count();
        std::minstd_rand generator(seed);

        // Generates random values for each element of the image.
        std::uniform_int_distribution<image_data_t> value_dist(0, 4);
        std::size_t img_elements = img.img_array.size();
        for (std::size_t i = 0; i < img_elements; i++) {
            img.img_array(i) = value_dist(generator);
        }
    }

    /**
     * Calculates the power of 2 of the smallest dimension in the image.
     */
    std::size_t find_max_l(const Image & img) {
        std::size_t max_l = SIZE_MAX;

        for (std::size_t i = 0; i < img.num_dims; i++) {
            std::size_t dim = eye::log2(img.img_array.shape(i));
            if (dim < max_l) {
                max_l = dim;
            }
        }

        return max_l;
    }

    /**
     * Administrates mode calculations and returns the downsampled image.
     */
    image_pair_t downsample_image(const Image & img) {
        std::size_t dim_size = 2;

        Image ds_img = create_reduced_image(img, dim_size);
        mode_array_t mode_array(ds_img.img_array.size());

        std::vector<std::future<mode_pair_t>> futures;

        ThreadPool tp(MAX_WORK_THREADS);

        auto f = [&](const std::vector<std::size_t> & positions,
            const std::size_t & index) {
            futures.push_back(tp.queue_task(find_mode, std::ref(img),
                index));
        };
        polytopic_loop(img.shape, f, 0, dim_size);

        tp.stop();

        std::size_t ds_index = 0;
        for (auto & future : futures) {
            mode_pair_t result_pair = future.get();
            mode_array[ds_index] = result_pair.first;
            ds_img.img_array(ds_index) = result_pair.second;
            ds_index++;
        }

        return std::make_pair(ds_img, mode_array);
    }

    /**
     * Reduces the mode calculations from a previous downsampling to produce
     * the next level of downsampling.
     */
    image_pair_t downsample_reduce(const image_pair_t & img_pair) {
        std::size_t dim_size = 2;

        const Image & img = img_pair.first;
        const auto & prev_mode_array = img_pair.second;

        Image ds_img = create_reduced_image(img, dim_size);
        mode_array_t mode_array(ds_img.img_array.size());

        std::vector<std::future<mode_pair_t>> futures;

        ThreadPool tp(MAX_WORK_THREADS);

        auto f = [&](const std::vector<std::size_t> & positions,
            const std::size_t index) {
            futures.push_back(tp.queue_task(reduce_modes, std::ref(img),
                std::ref(prev_mode_array), index));
        };
        polytopic_loop(img.shape, f, 0, dim_size);

        tp.stop();

        std::size_t ds_index = 0;
        for (auto & future : futures) {
            mode_pair_t result_pair = future.get();
            mode_array[ds_index] = result_pair.first;
            ds_img.img_array(ds_index) = result_pair.second;
            ds_index++;
        }

        return std::make_pair(ds_img, mode_array);
    }

    /**
     * Creates a new Image object to hold the downsampled image values.
     */
    Image create_reduced_image(const Image & img, const std::size_t dim_size) {
        // Reduce dimensions.
        std::vector<std::size_t> reduced_dims;
        for (std::size_t i = 0; i < img.num_dims; i++) {
            std::size_t reduced_dim_size = img.shape[i] / dim_size;
            if (reduced_dim_size > 1) {
                reduced_dims.push_back(reduced_dim_size);
            }
        }
        // If all of the dimensions are of length one, just collapse into a
        // single dimension.
        if (reduced_dims.empty()) {
            reduced_dims.push_back(1);
        }

        std::size_t * reduced_shape = &reduced_dims[0];
        std::size_t num_reduced_dims = reduced_dims.size();
        image_array_t reduced_img_array(reduced_shape,
            reduced_shape + num_reduced_dims);

        return Image(reduced_img_array);
    }

    /**
     * Calculates the mode of a specific subsection of the given image.
     */
    mode_pair_t find_mode(const Image & img,
            const std::size_t start_index) {
        mode_map_t mode_map;
        // Initialize so that the first item encountered will be set as mode.
        mode_map.insert(std::make_pair(0, 0));
        image_data_t mode = 0;

        auto f = [&](const std::vector<std::size_t> & positions,
            const std::size_t & index) {
            image_data_t key = img.img_array(index);

            // Keep a count of the values encountered to determine mode.
            if (mode_map.count(key) > 0) {
                mode_map[key]++;
            } else {
                // Encountered a new unique number, add it to the map.
                mode_map.insert(std::make_pair(key, 1));
            }

            // Update the mode as we count.
            if (mode_map[key] > mode_map[mode]) {
                mode = key;
            }
        };

        // Loop through processing window and count.
        std::vector<std::size_t> loop_shape(img.num_dims, 2);
        polytopic_loop(img.shape, loop_shape, f, start_index);

        if (mode_map.count(0) > 0) {
            mode_map.erase(0);
        }

        return std::make_pair(mode_map, mode);
    }

    mode_pair_t reduce_modes(const Image & img,
            const std::vector<mode_map_t> & mode_array,
            const std::size_t start_index) {
        mode_map_t reduced_mode_map;
        // Initialize so that the first item encountered will be set as mode.
        reduced_mode_map.insert(std::make_pair(0, 0));
        image_data_t mode = 0;

        auto f = [&](const std::vector<std::size_t> & positions,
            const std::size_t & index) {
            mode_map_t mode_map = mode_array[index];

            for (auto const & kv : mode_map) {
                // If key exists in both maps, add.
                if (reduced_mode_map.count(kv.first) > 0) {
                    reduced_mode_map[kv.first] += kv.second;
                } else {
                    reduced_mode_map.insert(std::make_pair(kv.first, kv.second));
                }

                if (reduced_mode_map[kv.first] > reduced_mode_map[mode]) {
                    mode = kv.first;
                }
            }
        };

        // Loop through processing window and reduce counts.
        std::vector<std::size_t> loop_shape(img.num_dims, 2);
        polytopic_loop(img.shape, loop_shape, f, start_index);

        if (reduced_mode_map.count(0) > 0) {
            reduced_mode_map.erase(0);
        }

        return std::make_pair(reduced_mode_map, mode);
    }
}
