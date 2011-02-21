// fallback implementation for log2_dot.s which is implemented specifically for 64-bit architectures

#include "interface/log2_dot.hpp"
#include <cstddef>

double log2_dot(const double * x, const double * y, unsigned int n){
    double result = 0.0;
    for(size_t i=0; i<n; ++i){
        result += y[i] * log2(x[i]);
    }
    return result;
}

