#ifndef PLUGIN_DELTANLL_HYPOTEST_HPP
#define PLUGIN_DELTANLL_HYPOTEST_HPP

#include "interface/decls.hpp"

#include "interface/plugin_so_interface.hpp"
#include "interface/database.hpp"
#include "interface/producer.hpp"

#include <string>

/** \brief Result table for the deltanll_hypotest producer
 *
 * The table contains the following columns:
 * <ol>
 * <li> runid INT(4)</li>
 * <li> eventid INT(4)</li>
 * <li> posterior_sb DOUBLE</li>
 * <li> posterior_b DOUBLE</li>
 * </ol>
 *
 * For every event, one entry is made, containing the result of the \link deltanll_hypotest deltanll_hypotest \endlink.
 */
class DeltaNLLHypotestTable: public database::Table {
public:
    /// Construct a DeltaNLLHypotestTable with name \c name_.
    DeltaNLLHypotestTable(const std::string & name_): database::Table(name_){}
    /// Save the \c nll_sb and \c nll_b values to the table using current \c runid and \c eventid from \c run.
    void append(const theta::Run & run, double nll_sb, double nll_b);
private:
    /// overrides Table::create_table
    virtual void create_table();
};

/** \brief A producer to create test statistics based on likelihood ratio in case of signal search.
 *
 * This producer assumes that you search for a signal and can use special parameter values in your model to define
 * the "background only" and "signal plus background" hypotheses.
 *
 * Configuration is done via a setting group like
 *<pre>
 * hypotest = {
 *   type = "deltanll_hypotest";
 *   minimizer = "myminuit";
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
 * The found values of the negativ log-likelihood are saved in the \c nll_sb and \c nll_b columns of the \link DeltaNLLHypotestTable result table \endlink.
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
    /// Construct a deltanll_hypotest Producer from a Configuration
    deltanll_hypotest(theta::plugin::Configuration & ctx);
    /// run the statistical method using \c data and \c model to construct the likelihood function and write out the result.
    virtual void produce(theta::Run & run, const theta::Data & data, const theta::Model & model);
private:
    theta::ParValues s_plus_b;
    theta::ParValues b_only;
    std::auto_ptr<theta::Minimizer> minimizer;
    theta::ParIds par_ids_constraints;
    DeltaNLLHypotestTable table;
};

#endif
