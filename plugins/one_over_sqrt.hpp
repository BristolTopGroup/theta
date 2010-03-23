#ifndef PLUGIN_ONE_OVER_SQRT
#define PLUGIN_ONE_OVER_SQRT

#include "interface/plugin_so_interface.hpp"
#include "interface/phys.hpp"

class one_over_sqrt: public theta::Function{
private:
    theta::ParId pid;

public:
    one_over_sqrt(const theta::plugin::Configuration & cfg);
    virtual double operator()(const theta::ParValues & v) const;
};


#endif
