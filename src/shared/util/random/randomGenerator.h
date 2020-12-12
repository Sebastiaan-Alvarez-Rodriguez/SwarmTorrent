#include <random>
#include <type_traits>

template <class T>
// Defines which uniform distribution to use
using uniform_distribution = 
typename std::conditional<
    std::is_floating_point<T>::value,
    std::uniform_real_distribution<T>, 
    typename std::conditional<
        std::is_integral<T>::value, 
        std::uniform_int_distribution<T>, 
        void
    >::type
>::type;
class RandomGenerator {
public:
    RandomGenerator(std::random_device rd) : gen(rd()) {};

    // Generates random number of type T in range [min, max]
    T generate(T min, T max) { uniform_distribution<T> dist(min, max); return dist(gen); };
private:
    std::mt19937 gen; 
};