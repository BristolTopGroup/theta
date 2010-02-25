#ifndef PLUGIN_CORE_HPP
#define PLUGIN_CORE_HPP

#include "interface/plugin_so_interface.hpp"
#include "interface/histogram-function.hpp"
//#include "libconfig/libconfig.h++"

class fixed_poly: public theta::ConstantHistogramFunction{
public:
   fixed_poly(theta::plugin::Configuration & cfg);
};

class fixed_gauss: public theta::ConstantHistogramFunction{
public:
   fixed_gauss(theta::plugin::Configuration & cfg);
};

#endif
