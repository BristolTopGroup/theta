#ifndef PLUGIN_MLE_HPP
#define PLUGIN_MLE_HPP

#include "interface/plugin_so_interface.hpp"
#include "interface/variables.hpp"
#include "interface/database.hpp"

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
  /** create a new MLETable with name \c name_
   * 
   * \parameter name is the name of the table in the database
   * \parameter vm is the VarIdManager used to translate ParId into column names
   * \parameter ids are the parameters to save in the table
   */
  MLETable(const std::string & name, const theta::VarIdManager & vm, const theta::ParIds & ids);
  /// append a new entry at the end of the table
  void append(const theta::Run & run, double nll, const theta::ParValues & values, const theta::ParValues & errors);
private:
    virtual void create_table();
    theta::ParIds save_ids;
    std::vector<std::string> pid_names;
};


/** \brief A maximum likelihood estimator.
 *
 * Given data and a model, this producer will find the maximum likelihood. It will write out the
 * parameters and errors at the minimum (as estimated by the minimizer) to the result table.
 *
 * In case the minimizer does not provide errors, -1 will be saved as error value.
 */
class MLEProducer: public theta::Producer{
public:
    /** \brief Constructor of MLEProducer
     *
     * \param vm is the VarIdManager used for the table column names
     * \param save_ids the parameters who's value at the minimum and error to save in the table
     * \param min Minimizer to use
     * \param name Name of the producer in the config file (for the table name).
     */
    MLEProducer(const theta::VarIdManager & vm, const theta::ParIds & save_ids, std::auto_ptr<theta::Minimizer> & min, const std::string & name_);
    virtual void produce(theta::Run & run, const theta::Data & data, const theta::Model & model);
private:
    std::auto_ptr<theta::Minimizer> minimizer;
    MLETable table;
};

/** \brief Factory class for MLEProducer.
 *
 * The Factory parses a setting group like
 *<pre>
 * {
 *   type = "maximum-likelihood";
 *   parameters = ("signal", "background");
 *   minimizer = "myminuit";
 * }
 * myminuit = {}; // minimizer definition
 *</pre>
 *
 * MLEProducer will write out results for each parameter given in the \c parameters list. For the table
 * format, see \MLETable.
 */
class MLEProducerFactory: public theta::plugin::ProducerFactory{
public:
    virtual std::auto_ptr<theta::Producer> build(theta::plugin::ConfigurationContext & ctx) const;
    virtual std::string getTypeName() const{
        return "maximum-likelihood";
    }
};

#endif
