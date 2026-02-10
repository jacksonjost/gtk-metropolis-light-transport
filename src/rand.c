#include "rand.h"
#include <stdlib.h>

//Note that randomness is currently deterministic as srand has not been called
Seed * generateSeed(){
    Seed * newSeed = malloc (sizeof(Seed));
    for (int index = 0; index < 4; ++ index) {
        uint64_t state = 0;
        for (int j = 0; j < 8; ++ j) {
            state = (state<<8) ^ (uint64_t)(rand() & 0xFF);
        }
        newSeed->state[index] = state;
    }

    if ((newSeed->state [0] | newSeed->state [1] | newSeed->state [2] | newSeed->state [3]) == 0) {
        newSeed->state[0] = 0xBAAAAAAD;
    }

    return newSeed;
}