#ifndef EYE_UTILITY_HPP
#define EYE_UTILITY_HPP

namespace eye {
    std::size_t pow(const std::size_t base, const std::size_t exp) {
        std::size_t result = 1;
        for (std::size_t i = 1; i <= exp; i++) {
            result *= base;
        }
        return result;
    }

    std::size_t log2(std::size_t v) {
        if (v == 0) return SIZE_MAX;
        if (v == 1) return 0;

        std::size_t l = 0;
        while (v > 1) {
            v >>= 1;
            l++;
        }

        return l;
    }
}
#endif
