#ifndef EYE_UTILITY_HPP
#define EYE_UTILITY_HPP

#include <sstream>
#include <string>
#include <vector>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

namespace eye {
    /**
     * Loop to traverse an n-dimensional polytopic structure.
     *
     * Note: Unfortunately, the stride is currently the step size across ALL
     * dimensions. Per-dimension stride specification is currently unavailable.
     */
    template<typename F>
    inline void polytopic_loop(
            const std::vector<std::size_t> & shape,
            F f,
            std::size_t index = 0,
            std::size_t stride = 1) {
        std::size_t num_dims = shape.size();
        std::vector<std::size_t> positions(num_dims, 0);
        std::size_t place = 0;
        bool overflow = false;

        while (!overflow) {
            // Execute loop logic here. Not very elegant TBH.
            f(positions, index);

            // Update position.
            positions[0] += stride;

            while (positions[place] >= shape[place]) {
                if (place == num_dims - 1) {
                    // Signal break out of outer loop.
                    overflow = true;
                    break;
                }

                // Carry the one.
                positions[place] = 0;
                place++;
                positions[place] += stride;
            }

            // Calculate the new index.
            index = positions[0];
            for (std::size_t i = 1; i < num_dims; i++) {
                index += positions[i] * shape[i - 1];
            }

            // Reset place counter.
            place = 0;
        }
    }

    /**
     * Retrieves the time as a formatted string.
     */
    inline std::string get_timestamp() {
        boost::posix_time::ptime local_time =
            boost::posix_time::second_clock::local_time();
        boost::posix_time::time_facet *facet =
            new boost::posix_time::time_facet("%Y%m%d%H%M%S");

        std::basic_stringstream<char> ss;
        ss.imbue(std::locale(ss.getloc(), facet));
        ss << local_time;
        return ss.str();
    }
}
#endif