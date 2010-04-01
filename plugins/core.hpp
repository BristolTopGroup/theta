#ifndef PLUGIN_CORE_HPP
#define PLUGIN_CORE_HPP

#include "interface/plugin.hpp"
#include "interface/histogram-function.hpp"
#include "interface/phys.hpp"
#include "interface/distribution.hpp"
#include "interface/matrix.hpp"

/** \brief A polynomial distribution where coefficients do not depend on any parameters
 *
 * Configuration is done with a setting group like
 * \code
 * {
 *  type = "fixed_poly";
 *  observable = "mass";
 *  normalize_to = 1.0;
 *  coefficients = [1.0, 2.0, 5.0];
 * };
 * \endcode
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
 * \code
 * {
 *  type = "fixed_gauss";
 *  observable = "mass";
 *  normalize_to = 1.0;
 *  mean = 1.0;
 *  width = 0.2;
 * };
 * \endcode
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
 * \code
 * {
 *  type = "log_normal";
 *  parameter = "p0";
 *  mu = 2.0;
 *  sigma = 0.5;
 * };
 * \endcode
 *
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
    virtual void sample(theta::ParValues & result, theta::Random & rnd) const;
    virtual double evalNL(const theta::ParValues & values) const;
    virtual double evalNL_withDerivatives(const theta::ParValues & values, theta::ParValues & derivatives) const;
    virtual const std::pair<double, double> & support(const theta::ParId&) const;
    virtual double width(const theta::ParId &) const;
    //@}
private:
    double mu, sigma;
    std::pair<double, double> support_;
};

/** \brief A truncated normal distribution in one or more dimensions
 *
 * A rectangular-truncated normal distribution.
 *
 * A one-dimensional case is configured with a setting group like 
 * \code
 * { 
 *  type = "gauss";
 *  parameter = "p0";
 *  range = (0.0, 5.0);
 *  mean = 2.0;
 *  width = 0.5;
 * };
 * \endcode
 *
 * \c parameter specifies the parameter the normal distribution depends on
 *
 * \c range is a list of doubles (or the special strings "inf", "-inf") specifying the truncation range.
 *
 * \c mean is a floating point value specifying the mean value of the distribution, \c width is its standard deviation.
 *
 * A multi-dimensional normal distribution can be specified with a setting like 
 * \code
 * { 
 *  type = "gauss";
 *  parameters = ("p0", "p1");
 *  ranges = ((0.0, 5.0), (0.0, "inf"));
 *  mean = [2.0, 3.0];
 *  covariance = ([1.0, 0.2], [0.2, 1.0]);
 * } 
 * \endcode
 *
 * \c parameters is a list of parameters this normal distribution depends on
 *
 * \c ranges is a list of ranges, in the same order as the parameters, which control the truncation
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

        //@{
        /** \brief Implementation of the pure methods of theta::Distribution
         *
         * See documentation of theta::Distribution.
         */
        virtual void sample(theta::ParValues & result, theta::Random & rnd) const;
        virtual void mode(theta::ParValues & result) const;
        virtual double evalNL(const theta::ParValues & values) const;
        virtual double evalNL_withDerivatives(const theta::ParValues & values, theta::ParValues & derivatives) const;
        virtual const std::pair<double, double> & support(const theta::ParId&) const;
        virtual double width(const theta::ParId &) const;
        //@}
    private:
        std::vector<theta::ParId> v_par_ids;
        std::vector<double> mu;
        theta::Matrix sqrt_cov; //required for sampling
        theta::Matrix inverse_cov;//required for density evaluation
        std::vector<std::pair<double, double> > ranges;
};

/** \brief A function which multiplies all its parameters
 *
 * For example, defining a Function to depend on ParId p0 and ParId p1,
 * operator(values) will always return values.get(p0)*values.get(p1).
 */
class mult: public theta::Function{
public:
    /** \brief Construct a MultFunction from a Configuration instance
        */
    mult(const theta::plugin::Configuration & cfg);

    /** \brief Definitions of the pure virtual methods of Function
     *
     * See documentation of Function for their meaning.
     */
    virtual double operator()(const theta::ParValues & v) const;
};

/** \brief A Distribution product of other distributions
 *
 * Product of zero or more Distributions. Configured via a setting group
 * like
 * \code
 *  {
 *     type = "product_distribution";
 *     distributions = ("@d1", "@d2");
 *  };
 *
 *  d1 = {...}; //some Distribution
 *  d2 = {...}; //some other Distribution
 * \endcode
 *
 * \c distributions contains a list of distribution specifications.
 *
 * It is not allowed that a parameter is provided by more than one distribution.
 */
class product_distribution: public Distribution{
public:

    /// Constructor from a Configuration for the plugin system
    product_distribution(const theta::plutgin::Configuration & cfg);

    //@{
    /// See Distribution for details
    virtual void sample(theta::ParValues & result, Random & rnd) const;
    virtual void mode(theta::ParValues & result) const;
    virtual double evalNL(const theta::ParValues & values) const;
    virtual double evalNL_withDerivatives(const theta::ParValues & values, theta::ParValues & derivatives) const;
    virtual const std::pair<double, double> & support(const theta::ParId & p) const;
    virtual double width(const theta::ParId & p) const;
    //@}

private:
    boost::ptr_vector<Distribution> distributions;
    std::map<ParId, size_t> parid_to_index;
};


#endif
