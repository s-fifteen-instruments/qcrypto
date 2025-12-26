#include <openssl/rand.h>
#include "rnd_openssl.h"

unsigned char buffer[4];

void set_PRNG_seed(unsigned int v) {
    return;
}

unsigned int PRNG_value(int v) {
    unsigned int mask = (1 << (unsigned int)v) - 1;
    int ret;
    do {
        ret = RAND_bytes(buffer, sizeof(buffer));
    } while (ret != 1);
    unsigned int result = (buffer[0] << 24) + (buffer[1] << 16) + (buffer[2] << 8) + buffer[3];
    return result & mask;
}
