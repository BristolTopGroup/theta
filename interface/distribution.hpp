#ifndef DISTRIBUTION_HPP
#define	DISTRIBUTION_HPP

//#include "boost/shared_ptr.hpp"
//#include "libconfig.h++"
#include "interface/variables.hpp"
#include "interface/matrix.hpp"

#include <memory>
#include <vector>

namespace libconfig{
    class Setting;
}

namespace theta{

    class AbsRandomProxy;

    /** \brief A class representing a probability distribution of parameters in one or more dimensions.
     * 
     * To prevent templating of this class with a random number generator type
     * as parameter, make use of \c AbsRandomProxy.
     */
    class Distribution{
    public:
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
        virtual void sample(ParValues & result, AbsRandomProxy & rnd, const VarIdManager & vm) const = 0;

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

        /** \brief Get the parameters this Distribution depends on.
         */
        ParIds getParameters() const{
            return par_ids;
        }
        virtual ~Distribution(){};
    protected:
        ParIds par_ids;
    };

    /** \brief A lognormal distribution in one variable.
     */
    class LogNormalDistribution: public Distribution{
    public:
        /** Construct a new LogNormal distribution in variable v_id with parameters
         * mu and sigma. Only sigma > 0 is allowed; otherwise, an InvalidArgumentException is thrown.
         */
        LogNormalDistribution(const ParId & p_id, double mu, double sigma);
        
        /** \brief Sample random values. 
         */
        virtual void sample(ParValues & result, AbsRandomProxy & rnd, const VarIdManager & vm) const;
        virtual double evalNL(const ParValues & values) const;
        virtual double evalNL_withDerivatives(const ParValues & values, ParValues & derivatives) const;
        virtual ~LogNormalDistribution(){}
    private:
        double mu, sigma;
    };
    
    class GaussDistribution: public Distribution{
    public:
        /** Construct a GaussDistribution which depends on the variable ids v_ids, have a mean of mu
         *  and a covariance matrix cov.
         *  If dimensions of v_ids, mu and cov mismatch, an InvalidArgumentException is thrown.
         *  If cov is not positive definite, a MathException is thrown.
         */
        GaussDistribution(const std::vector<ParId> & var_ids, const std::vector<double> & mu, const Matrix & cov);
        virtual void sample(ParValues & result, AbsRandomProxy & rnd, const VarIdManager & vm) const;
        virtual double evalNL(const ParValues & values) const;
        virtual double evalNL_withDerivatives(const ParValues & values, ParValues & derivatives) const;
        virtual ~GaussDistribution(){}

    private:
        //GaussDistribution(GaussDistribution & rhs);
        std::vector<ParId> v_par_ids;
        std::vector<double> mu;
        Matrix sqrt_cov; //required for sampling
        Matrix inverse_cov;//required for density evaluation
    };

}

#endif	/* _DISTRIBUTION_HPP */

