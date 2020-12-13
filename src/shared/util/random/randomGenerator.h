#ifndef SHARED_UTIL_RANDOM_RANDOMGENERATOR_H
#define SHARED_UTIL_RANDOM_RANDOMGENERATOR_H

#include <random>
#include <type_traits>

// Defines which uniform distribution to use

namespace rnd {
    template <class T>
    class RandomGenerator {
        using uniform_distribution = typename std::conditional<std::is_floating_point<T>::value, std::uniform_real_distribution<T>, typename std::conditional<std::is_integral<T>::value, std::uniform_int_distribution<T>, void>::type>::type;
    public:
        RandomGenerator(std::random_device& rd) : gen(rd()) {}

        // Generates random number of type T in range [min, max]
        inline T generate(T min, T max) { 
            uniform_distribution dist(min, max);
            return dist(gen);
        }

    private:
        std::mt19937 gen; 
    };
}
#endif