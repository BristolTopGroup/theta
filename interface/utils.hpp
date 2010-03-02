#ifndef UTILS_HPP
#define UTILS_HPP

#include <sys/time.h>
#include <algorithm>
#include <cmath>

#include <iostream>

#include <boost/math/special_functions/gamma.hpp>

namespace theta { namespace utils{

double diff_timeval(timeval * x, timeval * y);
double phi_inverse(double p);

/** \brief The lngamma function
 *
 * Forwards to the boost implementation which is thread save (note that
 * C99 implementations need not be therad save).
 */
inline double lngamma(double x){
   return boost::math::lgamma(x);
}

/** \brief possible redirections of log
 *
 * Tests have shown that the common log function is slow. In order
 * to allow easy switching for testing, all code in %theta should
 * use this log function.
 */
inline double log(double x){
    return ::log(x);
}

/** \brief Equality check for floating point numbers using relative comparison
 *
 * This function checks whether a and b are "close" on the sense
 * that the relative difference is very small.
 * a and b must not both be zero.
 */
inline bool close_to_relative(double a, double b){
   return fabs(a-b) / std::max(fabs(a),fabs(b)) < 10e-15;
}

/** \brief Equality check for floating point numbers using comparison to an external scale
 *
 * This function checks whether a and b are "close"
 * compared to \c scale. Note that \c scale is not a
 * maximal tolerance, but a typical scale which was used to 
 * compute a and b which might be equal as result of this
 * computation, but round-off errors might tell you that a!=b.
 *
 * scale > 0 must hold.
 */
inline bool close_to(double a, double b, double scale){
   return fabs(a-b) / scale < 10e-15;
}


}}

#endif
