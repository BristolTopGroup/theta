#ifndef PLUGIN_DELTANLL_INTERVALS_HPP
#define PLUGIN_DELTANLL_INTERVALS_HPP

#include "interface/variables.hpp"
#include "interface/database.hpp"
#include "interface/producer.hpp"

#include <string>

/** \brief Scan the likelihood function
 *
 *
 * Configuration is done with a settings block like:
 * \code
 * {
 *  type = "nll_scan";
 *  parameter = "p0";
 *  minimizer = "@myminimizer";
 *  parameter-values = {start = 0.0; stop = 1.0; n-steps = 101;};
 *
 *  re-minimize = false; //Optional. Default is true
 * }
 *
 * myminimizer = {...}; //see the minimizer documentation.
 * \endcode
 *
 * \c type has always to be "nll_scan" in order to use this producer
 *
 * \c parameter is the name of the parameter for which the interval shall be calculated.
 *
 * \c minimizer is the configuration path to a \link theta::Minimizer Minimizer\endlink definition.
 *
 * \c re-minimize specifies whether or not to search for a minimum of the negative log-likelihood when scanning
 *    through the parameter of interest or to use the parameter values found at the global minimum.
 *
 * \c parameter-values is a setting containing \c start, \c stop and \c step of the scan procedure for the
 *   given parameter. start < stop and stop >= 2 must hold.
 *
 * For the definition of the reduced likelihood function, see \ref deltanll_intervals, including the meaning of the
 * \c re-minimize parameter.
 *
 * The negative logarithm of the reduced likelihood function at the given \c parameter-values is written to a blob
 * column called "nll".
 *
 * The parameter value at the global minimum will be written to a column "maxl".
 */
class nll_scan: public theta::Producer{
public:

    /// \brief Constructor used by the plugin system to build an instance from settings in a configuration file
    nll_scan(const theta::plugin::Configuration & cfg);

    /** \brief Scan the reduced negative-log likelihood function and write out the result  to the database
     */
    virtual void produce(theta::Run & run, const theta::Data & data, const theta::Model & model);
    
    /** \brief Define the table columns
     *
     * Called by theta::Run::run as part of the setup. It defined the columns as described in the class documentation.
     */
    void define_table();
private:
    //boost::shared_ptr<theta::VarIdManager> vm;
    theta::ParId pid;
    double start, stop, step;
    unsigned int n_steps;
    bool re_minimize;
    
    //minimizer stuff:
    std::auto_ptr<theta::Minimizer> minimizer;
    bool start_step_ranges_init;
    theta::ParValues m_start, m_step;
    std::map<theta::ParId, std::pair<double, double> > m_ranges;
    
    //save the result here:
    std::vector<double> result;

    //table columns:
    std::auto_ptr<theta::Column> c_nll, c_maxl;
};

#endif

