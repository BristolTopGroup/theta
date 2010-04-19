#ifndef PLUGIN_ONE_OVER_SQRT
#define PLUGIN_ONE_OVER_SQRT

#include "interface/phys.hpp"

/** \brief Function returning 1/p where p is some parameter
 *
 * This is the example class discussed on the page \subpage extend "Extending theta".
 */
class one_over_sqrt: public theta::Function{
private:
    theta::ParId pid;

public:
    one_over_sqrt(const theta::plugin::Configuration & cfg);
    virtual double operator()(const theta::ParValues & v) const;
};


#endif
