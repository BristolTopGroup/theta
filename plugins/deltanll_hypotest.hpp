#ifndef PLUGIN_DELTANLL_HYPOTEST_HPP
#define PLUGIN_DELTANLL_HYPOTEST_HPP

#include "interface/decls.hpp"

#include "interface/plugin_so_interface.hpp"
#include "interface/database.hpp"
#include "interface/producer.hpp"

#include <string>

/** \brief A producer to create test statistics based on likelihood ratio in case of signal search.
 *
 * This producer assumes that you search for a signal and can use special parameter values in your model to define
 * the "background only" and "signal plus background" hypotheses.
 *
 * Configuration is done via a setting group like
 *<pre>
 * hypotest = {
 *   type = "deltanll_hypotest";
 *   minimizer = "@myminuit";
 *   background-only = {s = 0.0;}; //assuming "s" was defined as parameter earlier
 *   signal-plus-background = {};
 * };
 * 
 * myminuit = {...}; // minimizer definition
 *</pre>
 *
 * \c type is always "deltanll_hypotest" to select this producer.
 *
 * \c minimizer is the configuration path to a \link theta::Minimizer minimizer \endlink definition to be used
 *    for minimization of the negative-log-likelihood.
 *
 * \c background-only and \c signal-plus-background are setting groups which defines special parameter values which
 *   define the values to fix during the minimization, see below.
 *
 * Given data and a model, this producer will construct the negative-loglikelihood for the "signal-plus-background" parameters
 * fixed as specified in the configuration file and for the "background-only" parameters fixed and minimize the negative log-likelihood
 * with respect to all other parameters (while any constraints configured in the model apply, of course).
 *
 * The result table will contain the columns "nll_sb" and "nll_b", which contain the found value of the negative log-likelihood
 * for the "signal-plus-background" and "background-only" hypotheses, respectively.
 *
 * For a typical application, the "signal-plus-background" setting does not impose any fixed values and is the
 * empty settings group ("{};"), whereas the "background-only" settings group sets the signal to zero. Only
 * this case is considered in the following. Note that \c nll_sb <= \c nll_b <b>always</b> holds in this case as the minimization
 * is done using a larger set of parameters in the first case and the minimum cannot become larger. Any failure of this
 * unequality can only come from the numerical minimization of the likelihood not finding the correct minimum.
 *
 * If the number of observed events is large, \code 2 * sqrt(nll_b - nll_sb) \endcode will be a good estimate of
 * the significance (in sigma) with which the "background only" null-hypothesis "background-only" can be rejected. Even if you are not
 * in the asymptotic regime where this is true, you can sill use this test by generating the distribution of
 * \code 2 * sqrt(nll_b - nll_sb) \endcode for pseudo data generated under the "background only" assumption to define
 * the critical region in your particular case.
 */
class deltanll_hypotest: public theta::Producer{
public:
    /// \brief Constructor used by the plugin system to build an instance from settings in a configuration file
    deltanll_hypotest(const theta::plugin::Configuration & ctx);
    
    /// run the statistical method using \c data and \c model to construct the likelihood function and write out the result.
    virtual void produce(theta::Run & run, const theta::Data & data, const theta::Model & model);
    
    /// Define the table columns "nll_sb" and "nll_b" in the result table
    virtual void define_table();
private:
    theta::ParValues s_plus_b;
    theta::ParValues b_only;
    std::auto_ptr<theta::Minimizer> minimizer;
    theta::ParIds par_ids_constraints;
    theta::ProducerTable::column c_nll_b, c_nll_sb;
};

#endif
