#ifndef PLUGINS_SECANT_HPP
#define PLUGINS_SECANT_HPP

#include "interface/exception.hpp"

/** \brief The secant method to find the root of a one-dimensional function
 *
 * \param x_low The lower end of the start interval
 * \param x_high The higher end of the start interval
 * \param x_accuracy If the found interval is shorter than this, the iteration will stop
 * \param f_x_low is function(x_low).Used to save one function evalutation.
 * \param f_x_high is function(x_high). Used to save one function evaluation.
 * \param function The function object to use.
 *
 * Note that the function values at x_low and x_high must have different sign. Otherwise,
 * an InvalidArgumentException will be thrown.
 */
template<typename T>
double secant(double x_low, double x_high, double x_accuracy, double f_x_low, double f_x_high, const T & function){
    assert(x_low <= x_high);
    if(f_x_low * f_x_high >= 0) throw theta::InvalidArgumentException("secant: function values have the same sign!");
    
    const double old_interval_length = x_high - x_low;
    
    //calculate intersection point for secant method:
    double x_intersect = x_low - (x_high - x_low) / (f_x_high - f_x_low) * f_x_low;
    assert(x_intersect >= x_low);
    assert(x_intersect <= x_high);
    if(old_interval_length < x_accuracy){
        return x_intersect;
    }
    double f_x_intersect = function(x_intersect);
    double f_mult = f_x_low * f_x_intersect;
    //fall back to bisection if the new interval would not be much smaller:
    double new_interval_length = f_mult < 0 ? x_intersect - x_low : x_high - x_intersect;
    if(new_interval_length > 0.5 * old_interval_length){
        x_intersect = 0.5*(x_low + x_high);
        f_x_intersect = function(x_intersect);
        f_mult = f_x_low * f_x_intersect;
    }
    if(f_mult < 0){
        return secant(x_low, x_intersect, x_accuracy, f_x_low, f_x_intersect, function);
    }
    else if(f_mult > 0.0){
        return secant(x_intersect, x_high, x_accuracy, f_x_intersect, f_x_high, function);
    }
    //it can actually happen that we have 0.0. In this case, return the x value for
    // the smallest absolute function value:
    else{
        f_x_intersect = fabs(f_x_intersect);
        f_x_low = fabs(f_x_low);
        f_x_high = fabs(f_x_high);
        if(f_x_low < f_x_high && f_x_low < f_x_intersect) return x_low;
        if(f_x_high < f_x_intersect) return x_high;
        return x_intersect;
    }
}

#endif
