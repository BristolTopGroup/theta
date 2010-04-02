#ifndef PLUGIN_MLE_HPP
#define PLUGIN_MLE_HPP

//#include "interface/plugin_so_interface.hpp"
#include "interface/variables.hpp"
#include "interface/database.hpp"
#include "interface/producer.hpp"

#include <string>

/** \brief A maximum likelihood estimator.
 *
 * It is configuared via a setting group like
 *\code
 * {
 *   type = "mle";
 *   parameters = ("signal", "background");
 *   minimizer = "myminuit";
 * }
 * myminuit = {}; // minimizer definition
 *\endcode
 *
 * This producer will find the parameter values which minimize the negative-log-likelihood. It will write out these
 * parameter values. The error estimates provided by the \link theta::Minimizer minimizer \endlink are also written to the
 * table (note that these are -1 in case the minimizer does not provide errors). For the error estimate, the saverage
 * of the "plus" and "minus" errors will be used. These are often the same; consult the documentation of the
 * theta::Minimizer used for the error definition.
 *
 * The result table contains the column "nll", which holds the value of the negative log-likelihood at the found minimum,
 * and two columns for each of the given parameters, one with the estimated value of the parameter's name and one for
 * its error with appended "_error". For example, if the \c parameters given in the configuration file are ("signal", "background"),
 * the four columns "signal", "signal_error", "background" and "background" will be created.
 * 
 */
class mle: public theta::Producer{
public:
    
    /// \brief Constructor used by the plugin system to build an instance from settings in a configuration file
    mle(const theta::plugin::Configuration & cfg);
    
    /** \brief Run the method and write out results.
     */
    virtual void produce(theta::Run & run, const theta::Data & data, const theta::Model & model);
    
    /// Define the columns in the result table as specified in the class documentation
    virtual void define_table();
private:
    std::auto_ptr<theta::Minimizer> minimizer;
    //boost::shared_ptr<theta::VarIdManager> vm;
    std::vector<theta::ParId> save_ids;
    std::vector<std::string> parameter_names;
    
    bool start_step_ranges_init;
    theta::ParValues start, step;
    std::map<theta::ParId, std::pair<double, double> > ranges;
    
    //the two columns with parameter and error:
    std::vector<theta::EventTable::column> parameter_columns;
    std::vector<theta::EventTable::column> error_columns;
    theta::EventTable::column c_nll;
};

#endif
