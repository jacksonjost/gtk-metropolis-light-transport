#ifndef RAND_H
#define RAND_H

#include <stdint.h>

typedef struct {
    uint64_t  state[4];
} Seed;

Seed * generateSeed();

static inline uint64_t rotate (uint64_t number, int rotationDistance) {
    return (number << rotationDistance) |  (number >> (64- rotationDistance));
}

static inline void advanceState (uint64_t * s) {
    uint64_t t = s[1] << 17;
    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];

    s[2] ^= t;
    s[3] = rotate(s[3], 45);
}

static inline double randomDouble (Seed * seed){
    // Using xoshiro256+ for floating pt generation
    uint64_t * s = seed->state;
    uint64_t randomResult = s[0] + s[3];
    advanceState(s);
    return (double) (randomResult >> 11) * 0x1.0p-53; 
}

static inline uint64_t randomInt(Seed * seed){
    uint64_t *s = seed->state;
    uint64_t result = rotate (s[0] + s[3], 23) + s[0];
    advanceState(s);
    return result;
}

#endif