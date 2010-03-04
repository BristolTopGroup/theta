#ifndef PLUGIN_MCMC_QUANTILES_HPP
#define PLUGIN_MCMC_QUANTILES_HPP

#include "interface/decls.hpp"

#include "interface/plugin_so_interface.hpp"
#include "interface/database.hpp"
#include "interface/producer.hpp"
#include "interface/matrix.hpp"

#include <string>

/** \brief Result table for the mcmc_poterior_ratio.
 *
 * The table contains the following columns:
 * <ol>
 * <li> runid INT(4)</li>
 * <li> eventid INT(4)</li>
 * <li> for every n quantiles requested, there will n columns named quantile_&lt;i&gt; column, for i from 0 to n-1.</li>
 * </ol>
 *
 * For every event, one entry is made, containing the result of the \link mcmc_quantiles mcmc_quantiles \endlink.
 */
class mcmc_quantiles_table: public database::Table {
public:
    /// \brief Constructor used by the plugin system to build an instance from settings in a configuration file
    mcmc_quantiles_table(const std::string & name_): database::Table(name_){}
    
    /// Save the \c posterior_sb and \c posterior_b values to the table using current \c runid and \c eventid from \c run.
    void append(const theta::Run & run, const std::vector<double> & quantiles);
private:
    /// overrides Table::create_table
    virtual void create_table();
};

/** \brief Construct credible intervals based on Markov-Chain Monte-Carlo
 *
 * This producer integrates the posterior density and finds the given quantiles of a parameter.
 *
 * Configuration is done via a setting group like
 *<pre>
 * hypotest = {
 *   type = "mcmc_quantiles";
 *   parameter = "s";  //assuming "s" was defined as parameter earlier
 *   quantiles = [0.025, 0.16, 0.5, 0.84, 0.975];
 *   iterations = 10000;
 *   burn-in = 100; //optional. default is iterations / 10
 * };
 *
 *</pre>
 *
 * \c type is always "mcmc_posterior_ratio" to select this producer.
 *
 * \c parameter is the name of the parameter you want to find the quantiles for
 *
 * \c quantiles is a list or array of floating point values specifying the quantiles you want
 *
 * \c iterations is the number of MCMC iterations. See additional comments about runtime and suggested robustness tests
 *     in the documentation of \link mcmc_posterior_ratio mcmc_posterior_ratio \endlink.
 *
 * \c burn_in is the number of MCMC iterations to do at the beginning and throw away. See additional comments in the
 *     documentation of \link mcmc_posterior_ratio mcmc_posterior_ratio \endlink
 *
 * For each data given, one chain will be used to derive all quantile values given in the \c quantiles list, so they are
 * correlated by construction.
 */
class mcmc_quantiles: public theta::Producer{
public:
    /// \brief Constructor used by the plugin system to build an instance from settings in a configuration file
    mcmc_quantiles(theta::plugin::Configuration & ctx);
    
    /// run the statistical method using \c data and \c model to construct the likelihood function and write out the result.
    virtual void produce(theta::Run & run, const theta::Data & data, const theta::Model & model);
    
    /** \brief Return the contents for the information field in the ProducerInfoTable
     *
     * Will return a string with the space-separated quantiles in the order defined in the configuration and written to the
     * result table.
     */
    virtual std::string get_information() const;
private:
    //whether sqrt_cov* and startvalues* have been initialized:
    bool init;
    //if initialization failed, do not attempt to initialize again ...
    bool init_failed;
    
    //the requested quantiles:
    std::vector<double> quantiles;
    theta::ParId par_id;
    
    boost::shared_ptr<theta::VarIdManager> vm;
    
    size_t iterations;
    size_t burn_in;
    
    //the matrices and startvalues to use for the Markov chains:
    theta::Matrix sqrt_cov;
    std::vector<double> startvalues;
    
    mcmc_quantiles_table table;
};

#endif
