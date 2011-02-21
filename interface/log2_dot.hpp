#ifndef LOG2_DOT_HPP
#define LOG2_DOT_HPP

#include <cmath>

extern "C"{
    /** \brief Calculate inner product of a vector with log of the vector
     *
     * result = sum_i=0^{n-1}  y[i] * log2(x[i])
     */
    double log2_dot(const double * x, const double * y, unsigned int n);
}

#endif
