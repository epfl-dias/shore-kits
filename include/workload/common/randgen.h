/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __RANDGEN_H
#define __RANDGEN_H

#include "util.h" /* for ENTER_NAMESPACE/EXIT_NAMESPACE */

ENTER_NAMESPACE(workload);


class randgen_t {

    unsigned int _seed;
    
public:

    randgen_t(void* addr) {
        assert(sizeof(void*) >= sizeof(unsigned int));
        reset((unsigned int)addr);
    }

    randgen_t(unsigned int seed) {
        reset(seed);
    }

    void reset(unsigned int seed) {
        _seed = seed;
    }
    
    int rand() {
        return rand_r(&_seed);
    }
    
    /**
     * Returns a pseudorandom, uniformly distributed int value between
     * 0 (inclusive) and the specified value (exclusive).
     *
     * Source http://java.sun.com/j2se/1.5.0/docs/api/java/util/Random.html#nextInt(int)
     */
    int rand(int n) {
        assert(n > 0);

        if ((n & -n) == n)  // i.e., n is a power of 2
            return (int)((n * (uint64_t)rand()) / (RAND_MAX+1));

        int bits, val;
        do {
            bits = rand();
            val = bits % n;
        } while(bits - val + (n-1) < 0);
        
        return val;
    }
    
};


EXIT_NAMESPACE(workload);


#endif
