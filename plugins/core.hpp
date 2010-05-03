#ifndef PLUGIN_CORE_HPP
#define PLUGIN_CORE_HPP

#include "interface/plugin.hpp"
#include "interface/histogram-function.hpp"
#include "interface/phys.hpp"
#include "interface/distribution.hpp"
#include "interface/matrix.hpp"
#include "interface/database.hpp"

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
    virtual void mode(theta::ParValues & result) const;
    virtual double evalNL(const theta::ParValues & values) const;
    virtual double evalNL_withDerivatives(const theta::ParValues & values, theta::ParValues & derivatives) const;
    virtual const std::pair<double, double> & support(const theta::ParId&) const;
    virtual double width(const theta::ParId &) const;
    //@}
private:
    double mu, sigma;
    std::pair<double, double> support_;
};

/** \brief The delta distribution in one or more dimensions
 *
 * This distribution always returns the same values when sampled, which is equal to the mode.
 * The width is zero, the support only contains the fixed value.
 * 
 * Configuration is done via a setting group like
 * \code
 * {
 *   type = "delta_distribution";
 *   s = 1.0; 
 *   b = 2.0; //assuming "s" and "b" have been declared as parameters
 * };
 * \endcode
 * 
 * \c type must always be "delta_distribution" to create an instance of this class.
 * 
 * Further, for every parameter, the fixed value is given.  
 * 
 * The evalNL and evalNLwithDerivatives will always return 0. Evaluating them with parameters
 * other than the specified ones is a bug from the caller side. However, no diagnostic is
 * performed for efficiency reasons.
 */
class delta_distribution: public theta::Distribution{
public:
    /// \brief Constructor used by the plugin system to build an instance from settings in a configuration file
    delta_distribution(const theta::plugin::Configuration & cfg);
    
    //@{
    /** \brief Implementation of the pure methods of theta::Distribution
     *
     * See documentation of theta::Distribution. Note that evalNL and evalNL_withDerivatives will
     * always throw an IllegatStateException.
     */
    virtual void sample(theta::ParValues & result, theta::Random & rnd) const;
    virtual void mode(theta::ParValues & result) const;
    virtual double evalNL(const theta::ParValues & values) const;
    virtual double evalNL_withDerivatives(const theta::ParValues & values, theta::ParValues & derivatives) const;
    virtual const std::pair<double, double> & support(const theta::ParId&) const;
    virtual double width(const theta::ParId &) const;
    //@}
private:
    theta::ParValues values;
    std::map<theta::ParId, std::pair<double, double> > supports;
};

/** \brief A flat Distribution
 * 
 * This distribution sets ranges for parameters and returns all parameters with the same
 * probability on the given range.
 * 
 * Configuration is done via a setting group like
 * \code
 * {
 *   type = "flat_distribution";
 *   s = {
 *      range = (0.0, "inf");
 *      width = 1.0; //optional, but should be specified for infinite range
 *      fix-sample-value = 7.0; //optional, but should be specified for infinite range
 *   };
 *   
 *   b = {
 *     range = (0.0, 5.0);
 *     width = 2.0; //optional for finite range
 *     fix-sample-value = 2.0; //optional for finite range
 *   };
 * };
 * \endcode
 * 
 * \c type is always "flat_distribution" to select this type.
 * 
 * For every parameter this Distribution will be defined for, a setting group is given with
 * <ul>
 *   <li>a \c range setting which specifies, as a list of two doubles, the range of the variable.
 *     The special strings "inf" and "-inf" are allowed here.</li>
 *   <li>An optional \c fix-sample-value setting which will be used by the sample routine. In case of finite
 *     intervals, the default is to sample from a flat distribution on this interval. For infinite intervals,
 *     an exception will be thrown upon call of Distributiuon::sample</li>     
 *   <li>an optional \c width setting which will be used as starting step size for various algorithms
 *       (including markov chains and minimization). It should be set to a value of the same order of
 *       magnitude as the expected error of the likelihood function in this parameter. For finite intervals,
 *       the default used is 10% of the interval length. For infinite intervals with a non-zero
 *       \c fix-parameter-value setting, 10% of the absolute value is used. Otherwise, an exception will be thrown
 *       upon the call of flat_distribution::width().</li>
 * </ul>
 */
class flat_distribution: public theta::Distribution{
public:
    flat_distribution(const theta::plugin::Configuration & cfg);
    //@{
    /** \brief Implementation of the pure methods of theta::Distribution
     *
     * See class documentation and documentation of theta::Distribution for details.
     * Note that width, sample and mode might throw an exception for infinite intervals, as discussed
     * in the class documentation.
     */
    virtual void sample(theta::ParValues & result, theta::Random & rnd) const;
    virtual void mode(theta::ParValues & result) const;
    virtual double evalNL(const theta::ParValues & values) const;
    virtual double evalNL_withDerivatives(const theta::ParValues & values, theta::ParValues & derivatives) const;
    virtual const std::pair<double, double> & support(const theta::ParId&) const;
    virtual double width(const theta::ParId &) const;
    //@}
    
private:
    theta::ParValues fix_sample_values;
    theta::ParValues modes;
    theta::ParValues widths;
    std::map<theta::ParId, std::pair<double, double> > ranges;
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
    
private:
    std::vector<theta::ParId> v_pids;
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
class product_distribution: public theta::Distribution{
public:

    /// Constructor from a Configuration for the plugin system
    product_distribution(const theta::plugin::Configuration & cfg);

    //@{
    /// See Distribution for details
    virtual void sample(theta::ParValues & result, theta::Random & rnd) const;
    virtual void mode(theta::ParValues & result) const;
    virtual double evalNL(const theta::ParValues & values) const;
    virtual double evalNL_withDerivatives(const theta::ParValues & values, theta::ParValues & derivatives) const;
    virtual const std::pair<double, double> & support(const theta::ParId & p) const;
    virtual double width(const theta::ParId & p) const;
    //@}

private:
    void add_distributions(const theta::plugin::Configuration & cfg, const theta::SettingWrapper & s, int depth);
    
    boost::ptr_vector<theta::Distribution> distributions;
    std::map<theta::ParId, size_t> parid_to_index;
};

/** \brief A data source using a model
 *
 * Configured via a setting group like
 * \code
 * source = {
 *   type = "model_source";
 *   model = "@some-model-path";
 *   override-parameter-distribution = "@some-dist"; // optional
 *   save-nll = "distribution-from-override"; //optional, default is ""
 * };
 * \endcode
 *
 * \c type must be "model_source" to select this DataSource type
 *
 * \c model specifies the model to use for data creation
 *
 * \c override-parameter-distribution is an optional setting overriding the default behavior for parameter values, see below. It
 *     has to provide (at least) all the model parameters.
 *
 * \c save-nll is a string with allowed values "" (empty string), "distribution-from-override" and "distribution-from-model".
 *     Ifnot the empty string (""), the negative-log-likelihood of the sampled pseudo data will be saved to the event
 *     table for each call to fill() by constructing and evaluating the appropriate NLLikelihood.
 *
 * For each call of fill
 * <ol>
 *   <li>parameter values are sampled from the parameter distribution. If the setting \c override-parameter-distribution
 *       is given, it will be used for this step. Otherwise, the parameter distribution specified in the model will be used.</li>
 *   <li>the parameter values from the previous step are used in a call to call Model::samplePseudoData </li>
 *   <li>if \c save-nll is not the empty string (""), the likelihood function is built for the Data sampled
 *      in the previous step and evaluated at the parameters used sampled in step 1. As parameter distribution in the
 *      NLLikelihood instance, either the distribution from the model (if save-nll = "distribution-from-model") or the
 *      distribution specified in "override-parameter-distribution" (if save-nll = "distribution-from-override") is used.</li>
 * </ol>
 *
 * If \c save-nll is "distribution-from-override", \c override-parameter-distribution must be set.
 *
 * It is guaranteed that the method Distribution::sample is called exactly once per call to model_source::fill.
 * This can be used to specify a Distribution in the "override-parameter-distribution" setting which scans
 * some parameters (and only dices some others).
 * 
 * model_source will create a column in the event table for each model parameter and save the values used to sample the pseudo
 * data there. If save-nll is non-empty, a column "nll" will be created where the values will be saved.
 */
class model_source: public theta::DataSource{
public:

    /// Construct from a Configuration; required by the plugin system
    model_source(const theta::plugin::Configuration & cfg);

    /** \brief Fills the provided Data instance with data from the model
     *
     * Will only throw the DataUnavailable Exception if the parameter distribution instance
     * (i.e., either the model's instance or the override-parameter-distribution instance) throws an Exception.
     */
    virtual void fill(theta::Data & dat, theta::Run & run);
    
    virtual void define_table();

private:
    enum e_save_nll{ nosave, distribution_from_model, distribution_from_override };

    e_save_nll save_nll;
    
    std::auto_ptr<theta::Column> c_nll;
    
    theta::ParIds par_ids;
    std::vector<std::string> parameter_names;
    boost::ptr_vector<theta::Column> parameter_columns;
    
    std::auto_ptr<theta::Model> model;
    std::auto_ptr<theta::Distribution> override_parameter_distribution;

};

#endif

