#ifndef PLUGIN_MCMC_QUANTILES_HPP
#define PLUGIN_MCMC_QUANTILES_HPP

#include "interface/decls.hpp"

#include "interface/plugin_so_interface.hpp"
#include "interface/database.hpp"
#include "interface/producer.hpp"
#include "interface/matrix.hpp"

#include <string>

/** \brief Construct quantiles of the marginal posterior in one parameter based on Markov-Chain Monte-Carlo
 *
 * The result can be used to give upper limits or to construct symmetric credible intervals.
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
 * \c quantiles is a list or array of floating point values specifying the quantiles you want. Be aware that
 *   specifying very similar quantiles (with a difference less than 0.00001) is not allowed as this would yield same column
 *   name in the result table; see below.
 *
 * \c iterations is the number of MCMC iterations. See additional comments about runtime and suggested robustness tests
 *     in the documentation of \link mcmc_posterior_ratio mcmc_posterior_ratio \endlink.
 *
 * \c burn_in is the number of MCMC iterations to do at the beginning and throw away. See additional comments in the
 *     documentation of \link mcmc_posterior_ratio mcmc_posterior_ratio \endlink
 *
 * For each data given, one chain will be used to derive all requested quantiles given in the \c quantiles list, so their error
 * from limited chain length is correlated by construction. If you do not want that, use two independent producers of type
 * mcmc_quantiles.
 *
 * The result table contains as many columns as \c quantiles given in the configuration file. The column name
 * will be "quant" + 10000 * quantile, written with leading zeros. For example, if the quantile is 0.5,
 * the column name will be "quant05000", if the 99.9% quantile is requested (i.e., 0.999), the name will be "quant09990".
 */
class mcmc_quantiles: public theta::Producer{
public:
    /// \brief Constructor used by the plugin system to build an instance from settings in a configuration file
    mcmc_quantiles(const theta::plugin::Configuration & ctx);
    
    /// run the statistical method using \c data and \c model to construct the likelihood function and write out the result.
    virtual void produce(theta::Run & run, const theta::Data & data, const theta::Model & model);
    
    /// Define the table columns
    virtual void define_table();
    
private:
    //whether sqrt_cov* and startvalues* have been initialized:
    bool init;
    //if initialization failed, do not attempt to initialize again ...
    bool init_failed;
    
    //the requested quantiles:
    std::vector<double> quantiles;
    theta::ParId par_id;
    size_t ipar; //parameter of the requested index, as in NLLikelihood::operator()(const double*) index convention
    
    boost::shared_ptr<theta::VarIdManager> vm;
    
    //result columns: one per requested quantile:
    std::vector<theta::ProducerTable::column> columns;
    
    //MCMC parameters:
    unsigned int iterations;
    unsigned int burn_in;
    theta::Matrix sqrt_cov;
    std::vector<double> startvalues;
};

#endif
