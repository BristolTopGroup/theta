#ifndef PLUGIN_EXP
#define PLUGIN_EXP

#include "interface/phys.hpp"

/** \brief Function returning exp(lambda * p) where p is a configurable parameter and lambda a literal constant
 *
 */
class exp_function: public theta::Function{
private:
    theta::ParId pid;
    double lambda;

public:
    /// constructor for the plugin system
    exp_function(const theta::plugin::Configuration & cfg);
    /// overloaded evaluation operator of theta::Function
    virtual double operator()(const theta::ParValues & v) const;
};


#endif
