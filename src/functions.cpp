#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION

#include <chrono>
#include <iostream>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <andres/marray.hxx>
#include <boost/thread/future.hpp>
#include <eye/constants.hpp>
#include <eye/functions.hpp>
#include <eye/image.hpp>
#include <eye/math.hpp>
#include <eye/thread_pool.hpp>
#include <eye/utility.hpp>

typedef andres::Marray<int> image_array;

namespace eye {
    std::mutex write_mutex;

    Image generate_randomized_image(const std::size_t dims) {
        // Initialize random number generation.
        long int seed = std::chrono::high_resolution_clock::now()
            .time_since_epoch().count();
        std::minstd_rand generator(seed);

        // Generate randomized dimensionality for the image.
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

        // Initialize blank image.
        image_array img_array(shape, shape + dims);
        delete [] shape;

        // Generates random values for each element of the image.
        std::uniform_int_distribution<int> val_dist(0, 2);
        std::size_t img_size = img_array.size();
        for (std::size_t i = 0; i < img_size; i++) {
            int key = val_dist(generator);
            img_array(i) = key;
        }

        return Image(img_array);
    }

    void fill_image(Image & img) {
        // Initialize random number generation.
        long int seed = std::chrono::high_resolution_clock::now()
            .time_since_epoch().count();
        std::minstd_rand generator(seed);

        // Generates random values for each element of the image.
        std::uniform_int_distribution<int> value_dist(0, 2);
        std::size_t img_elements = img.img_array.size();
        for (std::size_t i = 0; i < img_elements; i++) {
            int key = value_dist(generator);
            img.img_array(i) = key;
        }
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

    /**
     * Administrates mode calculations and returns the downsampled image.
     */
    Image process_image(const Image & img, const std::size_t l) {
        std::size_t dim_size = eye::pow(2, l);

        // Define logic to be run inside polytopic loop.
        std::vector<std::size_t> starting_indices;
        auto f = [&](const std::vector<std::size_t> & positions,
            const std::size_t index) -> void {
            starting_indices.push_back(index);
        };
        eye::polytopic_loop(img.shape, f, 0, dim_size);

        // Create array to hold the values of the downsampled image.
        Image ds_img = create_reduced_image(img, dim_size);

        eye::ThreadPool tm(MAX_WORK_THREADS);
        std::vector<boost::future<void>> futures;

        // Kickoff tasks to find the mode of each processing window.
        std::size_t num_indices = starting_indices.size();
        for (std::size_t i = 0; i < num_indices; i++) {
            // Get mode of image section.
            auto f1 = tm.add_task(find_mode, std::ref(img), dim_size,
                starting_indices[i]);

            // Define future continuations to write results to downsampled image
            // asynchronously.
            auto f2 = f1.then([&tm, &ds_img, i](boost::future<int> f) {
                int mode = f.get();
                tm.add_task(write_result_to_image, std::ref(ds_img), i, mode);
            });

            futures.push_back(std::move(f2));
        }

        // Wait for all futures and tasks to return.
        boost::wait_for_all(futures.begin(), futures.end());
        tm.stop();

        return ds_img;
    }

    /**
     * Creates a new Image object to hold the downsampled image values.
     */
    Image create_reduced_image(const Image & img, const std::size_t dim_size) {
        // Get reduced dimensions.
        std::vector<std::size_t> reduced_dims;
        for (std::size_t i = 0; i < img.num_dims; i++) {
            std::size_t reduced_dim_size = img.shape[i] / dim_size;
            if (reduced_dim_size > 1) {
                reduced_dims.push_back(reduced_dim_size);
            }
        }
        if (reduced_dims.empty()) {
            reduced_dims.push_back(1);
        }

        // Convert shape data into array.
        std::size_t num_reduced_dims = reduced_dims.size();
        std::size_t * reduced_shape = new size_t[num_reduced_dims];
        for (std::size_t i = 0; i < num_reduced_dims; i++) {
            reduced_shape[i] = reduced_dims[i];
        }

        image_array reduced_img_array(reduced_shape, reduced_shape + num_reduced_dims);
        delete [] reduced_shape;

        return Image(reduced_img_array);
    }

    /**
     * Calculates the mode of a specific subsection of the given image.
     */
    int find_mode(const Image & img, const std::size_t dim_size,
            const std::size_t start_index) {
        // Bookkeeping for determining mode of processing window.
        std::map<int, int> mode_map;
        mode_map.insert(std::pair<int, int>(-1, -1));

        // Define logic to be run inside polytopic loop.
        auto f = [&](const std::vector<std::size_t> & positions,
            const std::size_t & index) -> void {
            int key = img.img_array(index);

            // Keep a count of the values encountered to determine mode.
            if (mode_map.count(key) > 0) {
                mode_map[key]++;
            } else {
                mode_map.insert(std::pair<int, int>(key, 1));
            }

            // Update the mode as we count.
            if (mode_map[key] > mode_map[mode_map[-1]]) {
                mode_map[-1] = key;
            }
        };

        std::vector<std::size_t> loop_shape;
        for (std::size_t i = 0; i < img.num_dims; i++) {
            loop_shape.push_back(dim_size);
        }
        eye::polytopic_loop(loop_shape, f, start_index);

        return mode_map[-1];
    }

    void write_result_to_image(Image & img, const int index, const int mode) {
        std::lock_guard<std::mutex> img_write_guard(write_mutex);
        img.img_array(index) = mode;
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
}
