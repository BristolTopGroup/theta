#ifndef RUN_HPP
#define RUN_HPP

//#include "interface/decls.hpp"

#include "interface/phys.hpp"
#include "interface/exception.hpp"
#include "interface/random.hpp"
#include "interface/producer.hpp"
#include "interface/database.hpp"
#include "interface/plugin_so_interface.hpp"
#include "interface/cfg-utils.hpp"
#include "interface/plugin.hpp"

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

/** \brief Represents a run, i.e., a chain of producer executions on pseudo data (or data).
 *
 *
 * Note that this is an abstract class, you cannot instantiate it directly but only subclasses of it.
 * If you are unsure, have a look at the documentation of \link plain_run plain_run\endlink which is the
 * simplest subclass.
 *
 * The configuration is done via a setting like:
 * <pre>
 * {
 *   type = "..."; //depends on which subclass you want
 *   result-file = "result/abc.db";
 *   producers = ("hypotest", "hupotest2");
 *   n-events = 10000;
 *   model = "gaussoverflat";
 *
 *   //optional:
 *   run-id = 2; //default is 1.
 *   seed = 15; //default is -1.
 *   log-level = "error";//default is "warning"
 *   log-report = false;//default is true
 * }
 *
 * hypotest = {...}; //some producer definition
 * hypotest2 = {...}; //some other producer definition
 * </pre>
 *
 * \c type must always be "plain" to create an instance of \c PlainRun.
 *
 * \c result-file gives the path to the result database file to create to save the result in
 *
 * \c producers is a list of configuration file paths which contain the
 *    definition of the producers to run.
 *
 * \c n-events is the number of pseudo experiments to run. This is ignored for some subclasses.
 *
 * \c model is the model used for pseudo data creation and which is passed to the producers.
 *    Note that you can provide two different models for these steps if you use the settings
 *    \c model-pseudodata and \c model-producers
 *
 * \c run-id is an optional setting and specifies which run-id to write to the result table. This setting
 *    might be removed in future versions of this plugin, as it is not really needed at this point.
 *
 * \c seed is the random number generator seed. It is mainly useful for debugging (i.e., reproducing a bug might require
 *    choosing a particular seed). The default setting -1 will generate a different seed each time and should be used as default.
 *
 * \c log-level controls the amount of logging information written to the log table: only log messages with a
 *      severity level equal to or exceeding the level given here are actually logged. Valid values are "error", "warning", "info"
 *      and "debug". Note that it is not possible to disable logging of error messages.
 *
 * \c log-report is a boolean specifying whether or not to print a logging report to standard output at
 *       the end of the run. This report summarizes how many messages there have been from any non-suppressed
 *       level. This allows for a quick check by the user whether everything went Ok or whether there have been obvious errors.
 *  
 *  Handling of result tables is done in the individual producers. Only run-wide tables
 *  are managed here, that is
 *  <ul>
 *   <li>A \link database::LogTable LogTable \endlink called 'log', where all log entries of the run are stored.</li>
 *   <li>A \link database::ProducerInfoTable ProducerInfoTable \endlink called 'prodinfo', where the list of configured
 *        producers are stored.</li>
 *   <li>A \link database::RndInfoTable RndInfoTable \endlink called 'rndinfo', where the random number generator
         seed used for this run is stored.</li>
 *   <li>A \link database::ParamTable ParamTable \endlink called 'params', where for each event (=pseudo experiment),
 *       the actually used parameter values used to generate the pseudo data from the pseusodata model are saved.</li>
 *  </ul>
 *
 *  For more information about these tables, refer to the documentation of the of the corresponding Table classes.
 */
class Run {
public:
    /// Define us as the base_type for derived classes; required for the plugin system
    typedef Run base_type;

   /** \brief Register progress listener.
    *
    * The registered progress listener will be informed about the current progress of the run during
    * execution of Run::run().
    */
    void set_progress_listener(const boost::shared_ptr<ProgressListener> & l);

    /** \brief Perform the actual run.
     * 
     * The actual meaning depends on the derived classes. Common to all is
     * that some pseudo experiments is performed whose data is
     * passed to each of the producers.
     */
    void run();

    /// declare destructor virtual as polymorphic access to derived classes is likely
    virtual ~Run() {}

    //@{
    /** \brief Get information about the current Run state
     *
     * These methods are meant to be used by the producers which are passed
     * a reference to the current run each time.
     */
    boost::shared_ptr<database::Database> get_database(){
       return db;
    }
    
    Random & get_random(){
        return rnd;
    }
    
    int get_runid() const{
        return runid;
    }

    int get_eventid() const{
        return eventid;
    }
    //@}

protected:
    
    /** \brief Construct a Run using the supplied configuration
     *
     */
    // protected, as Run is purley virtual.
    Run(const plugin::Configuration & cfg);
    
    /** \brief Add a producer to the list of producers to run.
    *
    * Add \c p to the internal list of producers. Memory ownership will be transferred,
    * i.e., p.get() will be 0 after this function returns.
    * 
    * It is only valid to call this function after creation, before any calls to the *run methods.
    * If violating this, the behavior is undefined.
    */
    void addProducer(std::auto_ptr<Producer> & p);
    
    /** \brief Actual run implementation
    *
    * This is the method to implement in subclasses of run. It is the method to implement
    * for derived classes which does all the work.
    */
    virtual void run_impl() = 0;
    
    /** \brief Make an informational log entry to indicate the start of a pseudo experiment.
    * 
    * Should be called by derived classes before pseudo data generation.
    */
    void log_event_start() {
        logtable->append(*this, database::severity::info, "start");
    }
    
    /** \brief Make an informational log entry to indicate the end of a pseudo experiment.
    * 
    * Should be called by derived classes after all producers have been run.
    */
    void log_event_end() {
        logtable->append(*this, database::severity::info, "end");
    }

    //random number generator seed and generator:
    unsigned int seed;
    Random rnd;
    
    boost::shared_ptr<VarIdManager> vm;

    //pseudo data:
    Data data;

    //can refer to the same Model:
    Model m_pseudodata;
    Model m_producers;

    //database and logtable as shared_ptr, as they are used by the producers:
    boost::shared_ptr<database::Database> db;
    boost::shared_ptr<database::LogTable> logtable;
    bool log_report;

    //the tables only used by run (and only by run):
    database::ProducerInfoTable prodinfo_table;
    database::RndInfoTable rndinfo_table;
    std::auto_ptr<database::ParamTable> params_table;

    //the producers to be run on the pseudo data:
    boost::ptr_vector<Producer> producers;

    
    //the runid, eventid and the total number of events to produce:
    int runid;
    int eventid;
    const int n_event;
    
    boost::shared_ptr<ProgressListener> progress_listener;
};


}

#endif
