#ifndef RUN_HPP
#define RUN_HPP

#include "interface/phys.hpp"
#include "interface/model.hpp"
#include "interface/exception.hpp"
#include "interface/random.hpp"
#include "interface/producer.hpp"
#include "interface/database.hpp"
#include "interface/cfg-utils.hpp"
#include "interface/plugin.hpp"
#include "interface/pm.hpp"

#include "libconfig/libconfig.h++"


#include <string>
#include <sstream>

#include <boost/ptr_container/ptr_vector.hpp>

namespace theta{

/** \brief A global flag indicating whether to stop execution as soon as possible
 *
 * All Run classes should check this flag regulary. If set to true, the run object
 * should return as soon as possible without violating the consistency of the result, i.e.,
 * the current (pseudo) data should be passed to all producers.
 *
 * This variable is set to true once theta receives SIGINT.
 *
 * It is defined in run.cpp.
 */
extern bool stop_execution;

/** \brief Install the SIGINT signal handler which sets stop_execution
 *
 * Calling this function will use the sigaction function to install a signal handler for
 * SIGINT which sets stop_execution to true to indicate to the run
 * to terminate as soon as possible. If stop_execution is already set to true,
 * exit(1) will be called immediately by the handler.
 */
void install_sigint_handler();

/** \brief A callback class called by the run to signal progress.
 *
 * User interfaces can derive their classes from  ProgressListener and implement
 * a visual feedback of the current state of the execution ("progress bar"). An instance
 * of this user-defined class should be registered with the
 * Run::set_progress_listener() method. Then, the Run instance will call ProgressListener::progress regularly
 * to report the current progress of the run.
 */
class ProgressListener{
public:
   /** \brief Indicate progress.
    *
    * This function will be called by Run to indicate the current progress.
    * \param done how many units of work have been done
    * \param total total units of work
    *
    * While Run objects should try to provide accurate feedback it is not guaranteed
    * that all "units of work" require the same time, nor it is guaranteed 
    * that the "total" amount of work units stays the same over time (actually, I assume you are
    * familiar with this from your own work).
    */
    virtual void progress(int done, int total) = 0;
};

/** \brief A set of producers executed on (pseudo-data)
 *
 * This is the central class in theta which "wires together" all required components and
 * implements the main algorithm of theta which consists of
 * <ol>
 *   <li>Get (pseudo-) data from a DataSource</li>
 *   <li>Pass this data to a list of producers which in turn write their results to the 'products' table</li>
 *   <li>Repeat steps 1 and 2 as many time as configured
 * </ol>
 *
 * Furthermore, this class serves as container for many objects shared across
 * one run, namly database and table objects, the random number generator, and the
 * (common) Model used by all producers.
 *
 * The configuration is done via a setting like:
 * \code
 * main = {
 *   data_source = {...}; //some data source definition
 *   output_database = {...}; // some database definition
 *
 *   producers = ("@hypotest", "@hypotest2");
 *   n-events = 10000;
 *   model = "@gaussoverflat";
 *
 *   //optional:
 *   seed = 15;  //default is -1.
 *   log-level = "error";  //default is "warning"
 *   log-report = false;  //default is true
 * };
 *
 * hypotest = {...}; //some producer definition
 * hypotest2 = {...}; //some other producer definition
 * gaussoverflat = {...}; // some model definition
 * \endcode
 *
 * \c data_source is a definition of a \link theta::DataSource DataSource \endlink implementation to use
 *    as source of the data / pseudodata to run the producers on
 *
 * \c output_database is a definition of a \link theta::Database Database \endlink implementation.
 *
 * \c producers is a list of links within the configuration file to setting groups which contain the
 *    definition of the producers to run.
 *
 * \c n-events is the number of pseudo experiments to run. This is ignored for some subclasses.
 *
 * \c model is the model used for the producers: the producers will be given this for pseudo data creation.
 *
 * \c seed is the random number generator seed. It is mainly useful for debugging (i.e.,
 *    reproducing a bug might require choosing a particular seed). The default setting -1 will generate a
 *    time-based seed and should be used as default.
 *
 * \c log-level controls the amount of logging information written to the log table: only log messages with a
 *      severity level equal to or exceeding the level given here are actually logged. Valid values
 *      are "error", "warning", "info" and "debug". Note that it is not possible to disable logging of messages with severity
 *      "error".
 *
 * \c log-report is a boolean specifying whether or not to print a logging report to standard output at
 *       the end of the run. This report summarizes how many messages there have been from any non-suppressed
 *       level. This allows for a quick check by the user whether everything went Ok or whether there
 *       have been obvious errors.
 *
 *  Handling of result tables is done in the individual producers. Only run-wide tables
 *  are managed here, that is
 *  <ul>
 *   <li>A \link LogTable LogTable \endlink called 'log', where all log entries are stored.</li>
 *   <li>A \link RndInfoTable RndInfoTable \endlink called 'rndinfo', where information about used random seeds by all plugin
 *     which consume randomness are saved. This can be useful if one wants to reproduce the exact same run (e.g., the same
 *     pseudo data between different theta runs or to provoke a certain rarely ocurring bug). </li>
 *   <li>A \link ProductsTable ProductsTable \endlink called 'events' where the per-event results from all
 *     the producers are stored and other per-event information.</li>
 *  </ul>
 *
 *  For more information about these tables, refer to the documentation of the corresponding Table classes.
 *
 * The RndInfoTable and ProductsTable are stored via the PropertyMap interface as "default" instances.
 */
class Run{
public:

   /** \brief Register progress listener.
    *
    * The registered progress listener will be informed about the current progress of the run during
    * execution of Run::run().
    */
    void set_progress_listener(const boost::shared_ptr<ProgressListener> & l);

    /** \brief Perform the actual run.
     * 
     * In a pseudo-experiment loop, ask the configuraed data_source for data and run
     * all the producers on it, using the configured model.
     */
    void run();
    
    /** \brief Initialize the members using the supplied configuration
     */
    void init(const plugin::Configuration & cfg);
    
private:
      
    /** \brief Make an informational log entry to indicate the start of a pseudo experiment.
     * 
     * Should be called by derived classes before pseudo data generation.
     */
    void log_event_start() {
        logtable->append(runid, eventid, LogTable::info, "start");
    }
    
    /** \brief Make an informational log entry to indicate the end of a pseudo experiment.
     * 
     * Should be called by derived classes after all producers have been run.
     */
    void log_event_end() {
        logtable->append(runid, eventid, LogTable::info, "end");
    }

    boost::shared_ptr<VarIdManager> vm;
    std::auto_ptr<Model> model;
    //note: the tables hold a shared_ptr to this database to ensure proper destruction order
    boost::shared_ptr<Database> db;
    std::auto_ptr<DataSource> data_source;

    std::auto_ptr<LogTable> logtable;
    bool log_report;
    boost::shared_ptr<RndInfoTable> rndinfo_table;

    //the producers to be run on the pseudo data:
    boost::ptr_vector<Producer> producers;
    boost::shared_ptr<ProductsTable> products_table;

    //the runid, eventid and the total number of events to produce:
    int runid;
    int eventid;
    int n_event;
    
    boost::shared_ptr<ProgressListener> progress_listener;
};


}

#endif

