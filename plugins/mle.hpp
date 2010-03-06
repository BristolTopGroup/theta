#ifndef PLUGIN_MLE_HPP
#define PLUGIN_MLE_HPP

#include "interface/plugin_so_interface.hpp"
#include "interface/variables.hpp"
#include "interface/database.hpp"
#include "interface/producer.hpp"

#include <string>


/** \brief Result table for the MLEProducer.
 *
 * The table contains the following columns:
 * <ol>
 * <li> runid INT(4)</li>
 * <li> eventid INT(4)</li>
 * <li> nll DOUBLE</li>
 * <li>for each configured parameter, a DOUBLE entry with its name and a DOUBLE entry with name &lt;varname&gt;_error </li>
 * </ol>
 *
 * For every event, one entry is made, containing the result of the MLEProducer. For more information
 * about the meaning, see documentation of MLEProducer.
 */
class MLETable: public database::Table {
public:
  /** \brief Create a new MLETable, given its name
   */
  MLETable(const std::string & name_): database::Table(name_){}
  
  /** \brief Initialize the table
   *
   * Before append(), this method must be called in order to ensure table creation with the correct columns.
   *
   * \param vm is the VarIdManager used to translate ParId into column names
   * \param ids are the parameters to save in the table
   */
  void init(const theta::VarIdManager & vm, const theta::ParIds & ids);
  
  /// append a new entry at the end of the table
  void append(const theta::Run & run, double nll, const theta::ParValues & values, const theta::ParValues & errors);
private:
    virtual void create_table();
    theta::ParIds save_ids;
    std::vector<std::string> pid_names;
};


/** \brief A maximum likelihood estimator.
 *
 * It is configuared via a setting group like
 *<pre>
 * {
 *   type = "mle";
 *   parameters = ("signal", "background");
 *   minimizer = "myminuit";
 * }
 * myminuit = {}; // minimizer definition
 *</pre>
 *
 * This producer will find the parameter values which minimize the negative-log-likelihood. It will write out these
 * parameter values. The error estimates provided by the \link theta::Minimizer minimizer \endlink are also written to the table (note that these are -1 in case
 * the minimizer does not provide errors). For the table format, see MLETable.
 */
class mle: public theta::Producer{
public:
    
    /// \brief Constructor used by the plugin system to build an instance from settings in a configuration file
    mle(const theta::plugin::Configuration & cfg);
    
    /** \brief Run the method and write out results.
     */
    virtual void produce(theta::Run & run, const theta::Data & data, const theta::Model & model);
private:
    std::auto_ptr<theta::Minimizer> minimizer;
    MLETable table;
};

#endif
