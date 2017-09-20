#ifndef EYE_UTILITY_HPP
#define EYE_UTILITY_HPP

#include <vector>

namespace eye {
    /**
     * Loop to traverse an n-dimensional polytopic structure.
     *
     * Note: Unfortunately, the stride is currently the step size across ALL
     * dimensions. Per-dimension stride specification is currently unavailable.
     */
    template<typename F>
    inline void polytopic_loop(
            const std::vector<std::size_t> & data_shape,
            const std::vector<std::size_t> & loop_shape,
            F f,
            const std::size_t start_index = 0,
            const std::size_t stride = 1) {
        std::size_t index = start_index;
        std::size_t num_dims = loop_shape.size();
        std::vector<std::size_t> positions(num_dims, 0);
        std::size_t place = 0;

        while (true) {
            // Execute loop logic here. Not very elegant TBH.
            f(positions, index);

            // Update position.
            positions[0] += stride;

            while (positions[place] >= loop_shape[place]) {
                if (place >= num_dims - 1) {
                    return;
                }

                // Carry the one.
                positions[place] = 0;
                place++;
                positions[place] += stride;
            }

            // Calculate the new index.
            index = start_index + positions[0];
            for (std::size_t i = 1; i < num_dims; i++) {
                index += positions[i] * data_shape[i - 1];
            }

            // Reset place counter.
            place = 0;
        }
    }

    inline std::size_t position_to_flat_index(
            const std::vector<std::size_t> & shape,
            const std::vector<std::size_t> & positions,
            const std::size_t start_index = 0) {
        std::size_t num_dims = shape.size();

        std::size_t index = start_index + positions[0];
        for (std::size_t i = 1; i < num_dims; i++) {
            index += positions[i] * shape[i - 1];
        }

        return index;
    }
}
#endif
