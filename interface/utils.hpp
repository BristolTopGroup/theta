#ifndef UTILS_HPP
#define UTILS_HPP

#include <sys/time.h>
#include <algorithm>
#include <cmath>

#define DEPRECATED __attribute__((__deprecated__))

//#include "crlibm.h"

#include <iostream>

#include <boost/math/special_functions/gamma.hpp>

namespace theta { namespace utils{

double diff_timeval(timeval * x, timeval * y);
double phi_inverse(double p);

inline double lngamma(double x){
   return boost::math::lgamma(x);
}

inline double log(double x){
    return ::log(x);
    //log_rn shows much better performance than ordinary log.
    //In my test case (-2lnQ distribution of qqhtt) it leads to an overall reduction
    // of execution time of about 5% which roughly corresponds to an improvement of
    // factor 2 in log performance alone
    //return log_rn(x);
}

//checks whether a and b are "close" on their own scale.
//a and b must not both be zero.
inline bool close_to_relative(double a, double b){
   return fabs(a-b) / std::max(fabs(a),fabs(b)) < 10e-15;
}

//checks whether a and b are close to each other
//given an absolute scale. Note that scale
//is *not* the maximal tolerance, but the scale to which
//the difference of the values should be *very* small to.
inline bool close_to(double a, double b, double scale){
   return fabs(a-b) / scale < 10e-15;
}


}}

#endif
