#include <chrono>
#include <iostream>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <andres/marray.hxx>
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
    std::vector<eye::Image> process_image(const Image & img) {
        // Find the power of 2 of the smallest dimension of the image.
        std::size_t min_l = eye::find_min_l(img);

        eye::ThreadPool tp(eye::MAX_WORK_THREADS);

        std::vector<std::future<eye::Image>> futures;

        // For each power of 2 from 1 to min_l, compute the downsampled image.
        for (std::size_t i = 1; i <= min_l; i++) {
            futures.push_back(tp.queue_task(eye::downsample_image,
                std::ref(img), i));
        }

        // Wait for all downsampling tasks to complete.
        tp.stop();

        // Grab images for return.
        std::vector<eye::Image> ds_images;
        for (auto & future : futures) {
            ds_images.push_back(future.get());
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
        eye::polytopic_loop(img.shape, f);

        outfile.close();
        std::cout << "Wrote output to " << filename << std::endl;
    }

    /**
     * Calculates the power of 2 of the smallest dimension in the image.
     */
    std::size_t find_min_l(const Image & img) {
        std::size_t min_l = SIZE_MAX;

        for (std::size_t i = 0; i < img.num_dims; i++) {
            std::size_t dim = eye::log2(img.img_array.shape(i));
            if (dim < min_l) {
                min_l = dim;
            }
        }

        return min_l;
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

        Image img(image_array(shape, shape + dims));
        delete [] shape;
        eye::fill_image(img);

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
     * Administrates mode calculations and returns the downsampled image.
     */
    Image downsample_image(const Image & img, const std::size_t l) {
        std::size_t dim_size = eye::pow(2, l);

        Image ds_img = create_reduced_image(img, dim_size);
        std::mutex write_mutex;

        std::size_t ds_index = 0;
        auto f = [&](const std::vector<std::size_t> & positions,
            const std::size_t index) -> void {
            int mode = find_mode(img, dim_size, index);
            {
                std::lock_guard<std::mutex> write_guard(write_mutex);
                ds_img.img_array(ds_index) = mode;
                ds_index++;
            }
        };
        eye::polytopic_loop(img.shape, f, 0, dim_size);

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
        image_array reduced_img_array(reduced_shape, reduced_shape + num_reduced_dims);

        return Image(reduced_img_array);
    }

    /**
     * Calculates the mode of a specific subsection of the given image.
     */
    image_data_t find_mode(const Image & img, const std::size_t dim_size,
            const std::size_t start_index) {
        // Bookkeeping for determining mode of processing window.
        std::map<image_data_t, std::size_t> mode_map;
        // Initialize so that the first item encountered will be set as mode.
        mode_map.insert(std::pair<image_data_t, std::size_t>(0, 0));
        image_data_t mode = 0;

        auto f = [&](const std::vector<std::size_t> & positions,
            const std::size_t & index) -> void {
            image_data_t key = img.img_array(index);

            // Keep a count of the values encountered to determine mode.
            if (mode_map.count(key) > 0) {
                mode_map[key]++;
            } else {
                mode_map.insert(std::pair<image_data_t, std::size_t>(key, 1));
            }

            // Update the mode as we count.
            if (mode_map[key] > mode_map[mode]) {
                mode = key;
            }
        };

        // Loop through processing window and count.
        std::vector<std::size_t> loop_shape;
        for (std::size_t i = 0; i < img.num_dims; i++) {
            loop_shape.push_back(dim_size);
        }
        eye::polytopic_loop(loop_shape, f, start_index);

        return mode;
    }
}
