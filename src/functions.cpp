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

        return downsample_image(img, 1, max_l);
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
        polytopic_loop(img.shape, img.shape, f);

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
            std::size_t dim_size = eye::pow(2, pow_dist(generator));
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
    std::vector<Image> downsample_image(const Image & img,
            const std::size_t l,
            const std::size_t max_l) {
        std::size_t dim_size = 2;

        Image ds_img = create_reduced_image(img, dim_size);

        std::size_t * mode_array_shape = &ds_img.shape[0];
        andres::Marray<mode_map_t> mode_array(mode_array_shape,
            mode_array_shape + ds_img.num_dims);

        std::size_t ds_index = 0;
        auto f = [&](const std::vector<std::size_t> & positions,
            const std::size_t & index) {
            mode_pair_t mode_pair = find_mode(std::ref(img), index);
            mode_array(ds_index) = mode_pair.first;
            ds_img.img_array(ds_index) = mode_pair.second;

            ds_index++;
        };
        polytopic_loop(img.shape, img.shape, f, 0, dim_size);

        std::vector<Image> ds_images;
        if (l < max_l) {
            ds_images.push_back(downsample_reduce(mode_array, ds_img, (l + 1),
                max_l, ds_images));
        }
        ds_images.push_back(ds_img);

        // Make image ordering more intuitive by converting to ascending level
        // of downsampling.
        std::reverse(ds_images.begin(), ds_images.end());

        return ds_images;
    }

    /**
     * 
     */
    Image downsample_reduce(const mode_array_t & prev_mode_array,
            const Image & img,
            const std::size_t l,
            const std::size_t max_l,
            std::vector<Image> & ds_images) {
        std::size_t dim_size = 2;

        Image ds_img = create_reduced_image(img, dim_size);

        std::size_t * mode_array_shape = &ds_img.shape[0];
        andres::Marray<mode_map_t> mode_array(mode_array_shape,
            mode_array_shape + ds_img.num_dims);

        std::size_t ds_index = 0;
        auto f = [&](const std::vector<std::size_t> & positions,
            const std::size_t index) {
            mode_pair_t mode_pair = reduce_modes(std::ref(prev_mode_array),
                index);
            mode_array(ds_index) = mode_pair.first;
            ds_img.img_array(ds_index) = mode_pair.second;
            ds_index++;
        };
        polytopic_loop(img.shape, img.shape,
            f, 0, dim_size);

        if (l < max_l) {
            ds_images.push_back(downsample_reduce(std::ref(mode_array), ds_img, (l + 1),
                max_l, ds_images));
        }

        return ds_img;
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
        // Bookkeeping for determining mode of processing window.
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

        if (mode_map[0] == 0) {
            mode_map.erase(0);
        }

        // Loop through processing window and count.
        std::vector<std::size_t> loop_shape(img.num_dims, 2);
        polytopic_loop(img.shape, loop_shape, f, start_index);

        return std::make_pair(mode_map, mode);
    }

    mode_pair_t reduce_modes(const mode_array_t & mode_array,
            const std::size_t start_index) {
        std::vector<std::size_t> mode_array_shape;
        for (std::size_t d = 0; d < mode_array.dimension(); d++) {
            mode_array_shape.push_back(mode_array.shape(d));
        }

        mode_map_t reduced_mode_map;
        // Initialize so that the first item encountered will be set as mode.
        reduced_mode_map.insert(std::make_pair(0, 0));
        image_data_t mode = 0;

        auto f = [&](const std::vector<std::size_t> & positions,
            const std::size_t & index) {
            auto mode_map = mode_array(index);

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

        std::vector<std::size_t> loop_shape(mode_array.dimension(), 2);
        polytopic_loop(mode_array_shape, loop_shape, f, start_index);

        if (reduced_mode_map[0] == 0) {
            reduced_mode_map.erase(0);
        }

        return std::make_pair(reduced_mode_map, mode);
    }
}
