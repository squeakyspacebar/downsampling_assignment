#ifndef EYE_CONSTANTS_HPP
#define EYE_CONSTANTS_HPP

#include <thread>

namespace eye {
    const unsigned int MAX_WORK_THREADS = std::thread::hardware_concurrency();
}
#endif
