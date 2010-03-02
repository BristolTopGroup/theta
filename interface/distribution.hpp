#ifndef DISTRIBUTION_HPP
#define DISTRIBUTION_HPP

#include "interface/decls.hpp"
#include "interface/variables.hpp"
//#include "interface/matrix.hpp"
//#include "interface/plugin_so_interface.hpp"

namespace theta{

    /** \brief A class representing a probability distribution of parameters in one or more dimensions.
     * 
     * To prevent templating of this class with a random number generator type
     * as parameter, make use of \c AbsRandomProxy.
     */
    class Distribution{
    public:
        /// Define us as the base_type for derived classes; required for the plugin system
        typedef Distribution base_type;
        
        /** Sample values from this distribution using \c rnd as random number generator
         *  and respecting limits set for the parameters in \c vm.
         *
         * The ParValues instance \c result will be used to set the random values.
         * This will set values for all variables this distribution
         * is defined on (i.e. all returned by getVariables()). Other parameter
         * values will not be touched.
         *
         * A VarIdManager instance has to be passed which holds range information.
         * Distribution ibjects must respect the range configured there for the
         * parameter.
         *
         * \param[out] result Fill the sampled values here.
         * \param rnd Proxy to the random number generator to use for sampling.
         * \param vm The VarIdManager instance which holds range values to respect.
         */
        virtual void sample(ParValues & result, Random & rnd, const VarIdManager & vm) const = 0;

        /** \brief The negative logarithm of the probability density.
         * 
         * The density is not guaranteed to be normalized, i.e., the negative
         * logarithm can be shifted by an arbitrary (but constant) value.
         *
         * \c values must contain (at least) the variable ids in getParameters().
         * Otherwise, a NotFoundException is thrown.
         * 
         * \param values The point in parameter space the density is to be evaluated.
         * \return The density at \c values.
         */
        virtual double evalNL(const ParValues & values) const = 0;
        
       /** \brief The nagetive logarithm of the probability, and derivatives thereof. 
        * 
        * Returns the negative logarithm of the prob. density at \c values, just as \c evalNL.
        * Additionally, the partial derivatives are filled into \c derivatives.
        * 
        * \param values The point in parameter space to use to evaluate the density and its derivatives.
        * \param[out] derivatives The container that will be filled with the partial derivatives.
        * \return The density at \c values. This is the same value as \c evalNL(values) would return.
        */
        virtual double evalNL_withDerivatives(const ParValues & values, ParValues & derivatives) const = 0;

        /** \brief Get the parameters this Distribution depends on and provides values for
         */
        ParIds getParameters() const{
            return par_ids;
        }
        
        /// declare destructor as virtual, as polymorhpic access will happen.
        virtual ~Distribution(){};
    protected:
        ParIds par_ids;
    };
}

#endif
