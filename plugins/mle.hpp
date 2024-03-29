#ifndef PLUGIN_MLE_HPP
#define PLUGIN_MLE_HPP

#include "interface/variables.hpp"
#include "interface/database.hpp"
#include "interface/producer.hpp"

#include <string>

/** \brief A maximum likelihood estimator
 *
 * It is configuared via a setting group like
 * \code
 * mle_producer = {
 *   type = "mle";
 *   parameters = ("signal", "background");
 *   minimizer = "@myminuit";
 *   write_covariance = true; //optional, default is false
 *   write_ks_ts = true; //optional, default is false
 * }
 * myminuit = {...}; // minimizer definition
 * \endcode
 *
 * \c type is always "mle"
 *
 * \c parameters is a list of parameters you want the maximum likelihood estimate for
 *
 * \c minimizer is a specification of a theta::Minimizer
 *
 * \c write_covariance controls whether the covariance matrix at the minimum is written to the
 *    result table. If set to true, a column of name 'covariance' of type typeHistogram is created.
 *    For n specified parameters, it will have n*n bins with range 0 to n*n. Matrix element at (i,j) will be
 *    at bin index i*n + j + 1 (assuming bin index convention where the lowest non-underflow bin has index 1).
 *    The exact meaning and construction of this matrix depends on the minimizer used; see documentation of the used Minimizer for details.
 *
 * \c write_ks_ts controls whether a Kolmogorov-Smirnov test statistic is written to the result table in column 'ks_ts'. If
 *   set to true, the KS test statistic is written to the products table. The KS test statistic is defined as the maximum
 *   absolute difference of the data and prediction cumulative distributions. The prediction from the model is evaluated using
 *   the parameter values from the maximum likelihood estimate. Note that the prediction histogram and data histograms are
 *   compared directly, no normalization is applied. If there is more than one observable, the KS test statistic is calculated for
 *   each observable and the maximum value is written to the products table.
 *
 * This producer uses the given minimizer to find the maximum likelihood estimates for the
 * configured parameters by minimizing the negative log-likelihood of the model, given data.
 *
 * <b>created columns</b> in the products table: for each parameter, two columns are created in the products table,
 * one with the parameter's name which contains the maximum likelihood estimate. The other
 * column with name '&lt;parameter name&gt;_error' contains the error estimate rom the minimizer
 * for that parameter. Additionally, one column is 'nll' is created which contains the value of the
 * negative log-likelihood at the minimum.
 */
class mle: public theta::Producer{
public:
    
    /// \brief Constructor used by the plugin system to build an instance from settings in a configuration file
    mle(const theta::plugin::Configuration & cfg);
    virtual void produce(const theta::Data & data, const theta::Model & model);
    
private:
    std::auto_ptr<theta::Minimizer> minimizer;
    std::vector<theta::ParId> save_ids;
    std::vector<std::string> parameter_names;
    
    bool start_step_ranges_init;
    theta::ParValues start, step;
    std::map<theta::ParId, std::pair<double, double> > ranges;
    
    bool write_covariance;
    bool write_ks_ts;
    bool write_bh_ts;

    std::auto_ptr<theta::ObsId> bh_ts_obsid;
    
    //the two columns with parameter and error:
    boost::ptr_vector<theta::Column> parameter_columns;
    boost::ptr_vector<theta::Column> error_columns;
    std::auto_ptr<theta::Column> c_nll;
    std::auto_ptr<theta::Column> c_covariance;
    std::auto_ptr<theta::Column> c_ks_ts;
    std::auto_ptr<theta::Column> c_bh_ts;
};

#endif
                                             
