#ifndef PLUGIN_MCMC_POSTERIOR_HISTO_HPP
#define PLUGIN_MCMC_POSTERIOR_HISTO_HPP

#include "interface/decls.hpp"
#include "interface/database.hpp"
#include "interface/producer.hpp"
#include "interface/random-utils.hpp"
#include "interface/matrix.hpp"

#include <string>

/** \brief Construct a Histogram of the marginalized Posterior in one parameter based on Markov-Chain Monte-Carlo
 *
 * Typically, this producer is called with a large number of iterations for only few pseudo
 * experiments.
 *
 * Configuration is done via a setting group like
 * \code
 * post = {
 *   type = "mcmc_posterior_histo";
 *   name = "post";
 *   parameters = ("s");  //assuming "s" was defined as parameter earlier
 *   iterations = 100000;
 *   burn-in = 100; //optional. default is iterations / 10 
 *   smooth = true; //optional, default is false
 *
 *   histo_s = {
 *      range = [0.0, 100.0];
 *      n_bins = 100;
 *   };
 * };
 *
 * \endcode
 *
 * \c type is always "mcmc_posterior_histo" to select this producer.
 *
 * \c name is a name chosen by the user used to construct unique column names in the result table (this name and two underscores are
 *   prepended to the column names explained below).
 *
 * \c parameters is a list of parameter names you want to calculate the posterior Histograms for
 *
 * \c iterations is the number of MCMC iterations. See additional comments about runtime and suggested robustness tests
 *     in the documentation of \link mcmc_posterior_ratio mcmc_posterior_ratio \endlink.
 *
 * \c burn_in is the number of MCMC iterations to do at the beginning and throw away. See additional comments in the
 *     documentation of \link mcmc_posterior_ratio mcmc_posterior_ratio \endlink
 *
 * \c smooth controls whether to make a smooth posterior histogram. It only works if specifying only one parameter.
 *  Please read important notes about this below. In particular, use smoothing <b>only</b> to make the final plot for the
 *  visualization of the marginal posterior; do <b>not</b> use it to calculate any quantitative results.
 *
 * For each parameter given in the \c parameters setting, you must specify a setting group
 * of name "histo_<parameter name>" which has two settings:
 * <ul>
 * <li>\c range is an arry of two floating point numbers defining the range of the Histogram</li>
 * <li>\c n_bins is an integer defining the number of bins of the Histogram.</li>
 * </ul>
 *
 * For each data given, one chain with the given number of iterations is constructed
 * and the value of the parameters is filled in the histogram. The histogram is
 * written to a column "posterior_<parameter name>".
 *
 * Without smoothing, the runtime is proportional to \c iterations + \c burn-in. It hardly depends on the number of histograms
 * or the number of bins per histogram configued.
 *
 * <b>Important notes about smoothing</b>
 *
 * Smoothing works by modifying the Markov Chain such that for each chain element, a whole histogram for the likelihood in the
 * parameter is calculated instead of a single point in parameter space. It is assumed that the histogram range is large enough
 * to extends to the tails of the posterior. If this is not the case, the result will have a bias.
 * Therefore, it is not recommended to use smoothed histograms for calculating any result (auch as quantiles) as the smoothing
 * is not accurate in all cases.
 *
 * The smoothing method also has important consequences for the runtime: the number of likelihood
 * evaluations per dataset without smoothing is about \c iterations  + \c burn-in, with smoothing it is about
 * ((\c iterations + \c burn-in)* \c n_bins ). Therefore, use a relatively small number of bins if enabling smoothing.
 * On the other hand, fewer iterations are necessary is enabling smoothing, so while you would typically use some millions
 * of iterations without smoothing in order to get 'smooth' plots, a few ten thousand iterations might be enough if you enable smoothing.
 */
class mcmc_posterior_histo: public theta::Producer, public theta::RandomConsumer{
public:
    /// \brief Constructor used by the plugin system to build an instance from settings in a configuration file
    mcmc_posterior_histo(const theta::plugin::Configuration & ctx);
    
    /// run the statistical method using \c data and \c model to construct the likelihood function and write out the result.
    virtual void produce(theta::Run & run, const theta::Data & data, const theta::Model & model);
    
private:
    //whether sqrt_cov* and startvalues* have been initialized:
    bool init;
    
    std::vector<theta::ParId> parameters;
    std::vector<std::string> parameter_names;
    std::vector<size_t> ipars; //parameters of the requested index, as in NLLikelihood::operator()(const double*) index convention
    
    //result columns: one per requested parameter:
    boost::ptr_vector<theta::Column> columns;
    std::vector<double> lower, upper;
    std::vector<size_t> nbins;
    
    //MCMC parameters:
    unsigned int iterations;
    unsigned int burn_in;
    theta::Matrix sqrt_cov;
    std::vector<double> startvalues;
    
    boost::shared_ptr<theta::VarIdManager> vm;
    
    bool smooth;
};

#endif
