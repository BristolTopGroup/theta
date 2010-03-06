#ifndef PLUGIN_CORE_HPP
#define PLUGIN_CORE_HPP

#include "interface/plugin_so_interface.hpp"
#include "interface/histogram-function.hpp"
#include "interface/distribution.hpp"
#include "interface/matrix.hpp"

/** \brief A polynomial distribution where coefficients do not depend on any parameters
 *
 * Configuration is done with a setting group like
 *<pre>
 * {
 *  type = "fixed_poly";
 *  observable = "mass";
 *  normalize_to = 1.0;
 *  coefficients = [1.0, 2.0, 5.0];
 * }
 *</pre>
 *
 * \c type must always be "fixed_poly" to construct an instance of this type
 *
 * \c observable is the name of a defined observable. This is used to construct a Histogram with the correct range and binning
 *
 * \c normalize_to is the sum of bin contents the returned histogram should have
 *
 * \c coefficients is an array (or list) of floating point values which define the polynomial, starting at x^0. The example above defines
 *  a polynomial 1 + 2*x + 5*x^2
 */
class fixed_poly: public theta::ConstantHistogramFunction{
public:
    /// \brief Constructor used by the plugin system to build an instance from settings in a configuration file
    fixed_poly(const theta::plugin::Configuration & cfg);
};

/** \brief A normal distribution where mean and width do not depend on any parameters
 *
 * Configuration is done with a setting group like
 *<pre>
 * {
 *  type = "fixed_gauss";
 *  observable = "mass";
 *  normalize_to = 1.0;
 *  mean = 1.0;
 *  width = 0.2;
 * }
 *</pre>
 *
 * \c type must always be "fixed_gauss" to construct an instance of this type
 *
 * \c observable is the name of a defined observable. This is used to construct a Histogram with the correct range and binning
 *
 * \c normalize_to is the sum of bin contents the returned histogram should have
 *
 * \c mean and \c width are the mean value and standard deviation for the distribution to construct.
 */
class fixed_gauss: public theta::ConstantHistogramFunction{
public:
    /// \brief Constructor used by the plugin system to build an instance from settings in a configuration file
   fixed_gauss(const theta::plugin::Configuration & cfg);
};

/** \brief A lognormal distribution in one dimension.
    *
    * It is configured with a setting group like
    * <pre>
    * {
    *  type = "log_normal";
    *  parameter = "p0";
    *  mu = 2.0;
    *  sigma = 0.5;
    * }
    * </pre> 
    * \c parameter specifies the parameter the normal distribution depends on
    *
    * \c mu and \c sigma are floating point constants used to define the distribution.
    * 
    * In the parametrization used, the density is proportional to:
    *   \f$ \exp( - (\ln(x) - \mu)^2 / (2 * \sigma^2) ) \f$
    * for x > 0 and 0 otherwise. x is the parameter the density depends on, mu and sigma are the configuration parameters.
    *
    */
class log_normal: public theta::Distribution{
public:
    /// \brief Constructor used by the plugin system to build an instance from settings in a configuration file
    log_normal(const theta::plugin::Configuration & cfg);
    
    //@{
    /** \brief Implementation of the pure methods of theta::Distribution
        *
        * See documentation of theta::Distribution.
        */
    virtual void sample(theta::ParValues & result, theta::Random & rnd, const theta::VarIdManager & vm) const;
    virtual double evalNL(const theta::ParValues & values) const;
    virtual double evalNL_withDerivatives(const theta::ParValues & values, theta::ParValues & derivatives) const;
    //@}
private:
    double mu, sigma;
};

/** \brief A Gaussian normal distribution in one or more dimensions. 
 * 
 * A one-dimensional case is configured with a setting group like 
 * <pre> 
 * { 
 *  type = "gauss"; 
 *  parameter = "p0"; 
 *  mean = 2.0; 
 *  width = 0.5; 
 * } 
 * </pre> 
 * \c parameter specifies the parameter the normal distribution depends on
 *
 * \c mean is a floating point value specifying the mean value of the distribution, \c width is its standard deviation.
 *
 * A multi-dimensional normal distribution can be specified with a setting like 
 * <pre> 
 * { 
 *  type = "gauss"; 
 *  parameters = ("p0", "p1"); 
 *  mean = [2.0, 3.0]; 
 *  covariance = ([1.0, 0.2], [0.2, 1.0]); 
 * } 
 * </pre> 
 *
 * \c mean specifies, in the same order as parameters, the mean values to use for the gaussian. 
 * 
 * \c covariance is the (symmetric) covariance matrix for the normal distribution. Note that 
 *     you give it as list of arrays (as it is symmetric anyway, it is left open what the "rows" and "columns" are). 
 */
class gauss: public theta::Distribution{
   public:
        /// \brief Constructor used by the plugin system to build an instance from settings in a configuration file
        gauss(const theta::plugin::Configuration & cfg);
        virtual void sample(theta::ParValues & result, theta::Random & rnd, const theta::VarIdManager & vm) const;
        virtual double evalNL(const theta::ParValues & values) const;
        virtual double evalNL_withDerivatives(const theta::ParValues & values, theta::ParValues & derivatives) const;
    private:
        std::vector<theta::ParId> v_par_ids;
        std::vector<double> mu;
        theta::Matrix sqrt_cov; //required for sampling
        theta::Matrix inverse_cov;//required for density evaluation
};

#endif
