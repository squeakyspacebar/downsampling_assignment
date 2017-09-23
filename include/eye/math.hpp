#ifndef EYE_MATH_HPP
#define EYE_MATH_HPP

namespace eye {
    inline std::size_t log2(std::size_t v) {
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
