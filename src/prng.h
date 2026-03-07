#ifndef SNAKE_PRNG_H__
#define SNAKE_PRNG_H__

namespace PRNG {
    unsigned rand();
    void srand(unsigned seed);
    constexpr int MAX = 32767;
}

#endif
