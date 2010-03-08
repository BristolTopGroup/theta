#ifndef PLUGIN_DELTANLL_INTERVALS_HPP
#define PLUGIN_DELTANLL_INTERVALS_HPP

#include "interface/plugin_so_interface.hpp"

#include "interface/variables.hpp"
#include "interface/database.hpp"
#include "interface/producer.hpp"

#include <string>

/** \brief Confidence intervals based on the difference in the negative log-likelihood.
 *
 * Configuration is done with a settings block like:
 * <pre>
 * {
 *  type = "delta-nll";
 *  parameter = "p0";
 *  clevels = [0.68, 0.95];
 *  minimizer = "myminuit";
 *  re-minimize = false; //Optional. Default is true
 * }
 *
 * myminuit = {...} //see the minimizer documentation.
 * </pre>
 *
 * \c type has always to be "delta-nll" in order to use this producer
 *
 * \c parameter is the name of the parameter for which the interval shall be calculated.
 *
 * \c clevels is an array (or list) of doubles of confidence levels for which the intervals
 *   shall be calculated. Note that an interval for the "0" confidence level (i.e., the
 *   interval containing the minimum) is always determined.
 *
 * \c minimizer is the configuration path to a \link theta::Minimizer Minimizer\endlink definition.
 *
 * \c re-minimize specifies whether or not to search for a minimum of the negative log-likelihood when scanning
 *    through the parameter of interest or to use the parameter values found at the global minimum. See below for details.
 *
 * This producer uses the likelihood to derive confidence intervals based on the
 * based on the asymptotic property of the likelihood ratio test statistics to be distributed
 * according to a chi^2-distribution.
 *
 * Given a likelihood function L which depends on parameters p_0 ... p_n, for which a
 * confidence interval for p0 should be constructed, the method works as follows:
 * <ol>
 * <li>vary parameters p_0 ... p_n to find the maximum value of the likelihood function, L_max, and the parameter
 *     values at the maximum.</li>
 * <li>starting from the value at the maximum, scan through the parameter p0 in two passes:
 *     once to lower and once to higher values of p0.
 *     For each new value of p0, wither (i) maximize L with respect to all other parameters, or (ii) using the
 *     values found in step 1 for the other parameters. Which method is used depends on the \c re-minimize setting.
 *     In both cases, one effectively has a function depending on p0 only, L_m(p_0).</li>
 * <li>The interval for p0 is given by the points where the ratio L_m(p0) / L_max corresponds to
 *     the desired confidence level.</li>
 * </ol>
 *
 * L_m(p_0) is sometimes called the "profiled likelihood function" or the "reduced likelihood function",
 * as it only depends on 1 of the original n+1 parameters.
 *
 * For each confidence level c, the result table contains columns "lower" + 10000*c and "upper" + 10000*c, where
 * the numbers are rounded and written with leading zeros. For example, if the \c clevels setting is [0.68, 0.95], the
 * column names will be "lower06800", "upper06800", "lower09500", "upper09500". Additionally, the table always contains
 * the parameter value at the maximum of the likelihood as "maxl". The "maxl" value is the same as you would get for
 * a confidence level of exactly 0.
 */
class deltanll_intervals: public theta::Producer{
public:

    /// \brief Constructor used by the plugin system to build an instance from settings in a configuration file
    deltanll_intervals(const theta::plugin::Configuration & cfg);

    /** \brief Run the statistical method with Data and model and write out the result  to the database.
     *
     * As this producer can be configured to determine several intervals at once, it
     * usually makes more than one entry per pseudo experiment in the result table.
     */
    virtual void produce(theta::Run & run, const theta::Data & data, const theta::Model & model);
    
    /** \brief Define the table columns
     *
     * Called by theta::Run::run as part of the setup. It defined the columns as described in the class documentation.
     */
    void define_table();
private:
    boost::shared_ptr<theta::VarIdManager> vm;
    theta::ParId pid;
    std::vector<double> clevels;
    bool re_minimize;
    //at construction, save here the deltanll values corresponding to
    //clevels:    
    std::vector<double> deltanll_levels;
    std::auto_ptr<theta::Minimizer> minimizer;

    //table columns:
    std::vector<theta::ProducerTable::column> lower_columns;
    std::vector<theta::ProducerTable::column> upper_columns;
    theta::ProducerTable::column c_maxl;
};

#endif

