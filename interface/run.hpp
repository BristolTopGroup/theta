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

/** \brief Represents a single run, i.e., a type="run" setting block in the configuration.
 *  
 *  Different "mode" settings are implemented via subclassing the run object.
 *  
 *  After creation of a Run object, following operations can be done with it, in this order:
 *  <ol>
 *   <li>call \c addProducer to add one or more producers </li>
 *   <li>call \c pre_run </li>
 *   <li>call \c run </li>
 *   <li>call \c post_run </li>
 *  </ol>
 *  Calling these functions in any other sequence will result in an IllegalStateException
 *  to be thrown. In particular, it is not possible to execute a run twice using
 *  the same instance of the Run class.
 *  
 *  Handling of result tables is done in the individual producers. Only run-wide tables
 *  are managed here, that is
 *  <ul>
 *   <li>A database::LogTable called 'log', where all log entries of the run are stored.</li>
 *   <li>A database::ProdInfoTable called 'prodinfo', where the list of configured producers for this run is stored.</li>
 *   <li>A database::RndInfoTable called 'rndinfo', where the random number generator configuration for this run is stored.</li>
 *   <li>A database::ParamInfoTable called 'params', where for each pseudo experiment,
 *       the actually used parameter values used to generate the pseudo data from 
 *       the pseusodata model are saved.</li>
 *  </ul>
 *
 *  For more information about these tables, see their documentation.
 */
template<typename rndtype>
class RunT {
public:
    typedef RunT<rndtype> base_type;

   /** \brief Register progress listener.
    *
    */
    void set_progress_listener(const boost::shared_ptr<ProgressListener> & l){
        progress_listener = l;
    }

    /** \brief The pre-run, to be executed before the \c run routine.
     * 
     * This routine performs common pre-run tasks like populating the informational
     * tables.
     */
    void pre_run() {
        if(state!=0){
            throw IllegalStateException();
        }
        state = 1;
        //record all producers in prodinfo_table:
        for(size_t i=0; i<producers.size(); i++){
            //TODO: save the setting(?!)
            prodinfo_table.append(*this, (int)i, producers[i].getName(), "");
        }
        //write random seeds to rndinfo_table:
        rndinfo_table.append(*this, seed);
        //call any pre_run method from the derived classes:
        pre_run_impl();
        log_run_start();
        state = 2;
    }
    
    /** \brief Perform the actual run.
     * 
     * The actual meaning depends on the derived classes. Common to all is
     * that a number of pseudo experiments is performed whose data is
     * passed to each of the producers.
     */
    void run(){
        if(state!=2){
            throw IllegalStateException();
        }
        state = 3;
        run_impl();
        state = 4;
    }
    
    /** \brief Perform any post-run cleaup. To be called after \c run.
     * 
     * This routine performs common post run tasks.
     */
    void post_run() {
        if(state!=4){
            throw IllegalStateException();
        }
        state = 5;
        log_run_end();
        //delegate to derived class:
        post_run_impl();
        state = 6;
    }

    /** Add \c p to the internal list of producers. Pointer ownership will be transferred,
     * i.e., p.get() will be 0 after this function returns.
     * 
     * It is only valid to call this function after creation, before any calls to the *run methods.
     * If violating this, the behavior is undefined.
     */
    void addProducer(std::auto_ptr<Producer> & p) {
        if(state!=0){
            throw IllegalStateException();
        }
        producers.push_back(p);
    }

    virtual ~RunT() {
        //db.close(); //not necessary, as is closed on destruction of db anyway (and we could not propagate exceptions from here anyway ...)
    }

    /** Get the Databse object associated to this run, i.e.,
     *  this is where the producers should create their result tables.
     */
    boost::shared_ptr<database::Database> get_database(){
       return db;
    }

    /// Get the currect run id. Will always be the same for the same instance of Run
    int get_runid() const{
        return runid;
    }

    /// Get the currently processed event id.
    int get_eventid() const{
        return eventid;
    }

protected:
    
    /** \brief Run constructor.
     *
     * \param seed Seed for the random number generator (used for pseudo data generation)
     * \param m_pseudodata_ Pseudodata model,  used to generate pseudo data
     * \param m_producers_ Model passed to the producers to make statistical inferences.
     * \param outfilename Path to the file system name of the output database.
     * \param runid_ The run id used in the output database.
     * \param n_event_ number of events (=number of pseudo experiments to perform).
     */
    // protected, as Run is purley virtual.
    RunT(plugin::Configuration & cfg): seed(-1), m_pseudodata(cfg.vm), m_producers(cfg.vm), db(new database::Database(cfg.setting["result-file"])),
      logtable(new database::LogTable("log")), prodinfo_table("prodinfo"), rndinfo_table("rndinfo"),
      runid(1), eventid(0), n_event(cfg.setting["n-events"]), state(0){
          
      const libconfig::Setting & s = cfg.setting;
      if(s.lookupValue(static_cast<const char*>("run-id"), runid)){
         cfg.rec.markAsUsed(s["run-id"]);
      }
      int i_seed = -1;
      if(s.lookupValue(static_cast<const char*>("seed"), i_seed)){
         cfg.rec.markAsUsed(s["seed"]);
      }
      seed = i_seed;
      if(seed==-1) seed = time(0);
      rnd.setSeed(seed);
      if (s.exists("model")) {
        std::string model_path = s["model"];
        plugin::Configuration context(cfg, cfg.rootsetting[model_path]);
        std::auto_ptr<Model> model = ModelFactory::buildModel(context);
        m_pseudodata = *model;
        m_producers = *model;
        cfg.rec.markAsUsed(s["model"]);
      }
      if (s.exists("model-pseudodata")) {
         std::string model_path = s["model-pseudodata"];
         plugin::Configuration context(cfg, cfg.rootsetting[model_path]);
         m_pseudodata = *ModelFactory::buildModel(context);
         cfg.rec.markAsUsed(s["model-pseudodata"]);
         s["model-producers"];//will throw if not found
       }
       if (s.exists("model-producers")) {
         std::string model_path = s["model-producers"];
         plugin::Configuration context(cfg, cfg.rootsetting[model_path]);
         m_producers = *ModelFactory::buildModel(context);
         cfg.rec.markAsUsed(s["model-producers"]);
         s["model-pseudodata"];//will throw if not found
       }
       
      logtable->connect(db);
      prodinfo_table.connect(db);
      rndinfo_table.connect(db);
      params_table.reset(new database::ParamTable("params", m_pseudodata.getVarIdManager(),
                                               m_pseudodata.getParameters(), m_pseudodata.getObservables()));
      params_table->connect(db);
      eventid = 0;
      
      //add the producers:
      int n_p = s["producers"].getLength();
      if (n_p == 0)
         throw ConfigurationException("no producers in run specified!");
      for (int i = 0; i < n_p; i++) {
        std::string prod_path = s["producers"][i];
        plugin::Configuration cfg2(cfg, cfg.rootsetting[prod_path]);
        std::auto_ptr<Producer> p = plugin::PluginManager<Producer>::build(cfg2);
        addProducer(p);
        //should transfer ownership:
        assert(p.get()==0);
      }
      cfg.rec.markAsUsed(s["producers"]);
    }    
    
    //random number generators for seeding (s) and pseudo data generation (g),
    long seed;
    rndtype rnd;

    //pseudo data:
    Data data;

    //can refer to the same Model:
    Model m_pseudodata;
    Model m_producers;

    //database and logtable as shared_ptr, as they are used by the producers:
    boost::shared_ptr<database::Database> db;
    boost::shared_ptr<database::LogTable> logtable;

    //the tables only used by run (and only by run):
    database::ProducerInfoTable prodinfo_table;
    database::RndInfoTable rndinfo_table;
    std::auto_ptr<database::ParamTable> params_table;

    //the producers to be run on the pseudo data:
    boost::ptr_vector<Producer> producers;

    int runid;
    
    //the current eventid:
    int eventid;
    const int n_event;
    
    boost::shared_ptr<ProgressListener> progress_listener;
    
    /** The \c pre_run, \c run and \c post_run methods will call
     * their *_impl counterpart which should be overriden in the derived classes.
     * 
     * At least \c run_impl *must* be implemented by derived classes.
     * */
    virtual void pre_run_impl() {}
    virtual void post_run_impl() {}
    virtual void run_impl() = 0;

    /** \brief Fill \c data with pseudo data from the model m_pseudodata and write
     * the used values of the parameters to the "params" table. In addition the
     * integral of the PEdata histo is written into the last column.
     */
    void fill_data() {
        ParValues values = m_pseudodata.sampleValues(rnd);
        data = m_pseudodata.samplePseudoData(rnd, values);
	const ObsIds & obs_ids = data.getObservables();
        std::map<theta::ObsId, double> n_data;
	for(ObsIds::const_iterator obsit=obs_ids.begin(); obsit!=obs_ids.end(); obsit++){
	    n_data[*obsit] = data.getData(*obsit).get_sum_of_bincontents();
	}
        params_table->append(*this, values, n_data);
    }

    /** \brief Make an informational log entry to indicate the start of a pseudo experiment.
     * 
     * Should be called before pseudo data generation.
     */
    void log_event_start() {
        logtable->append(*this, database::severity::info, "start");
    }
    
    /** \brief Make an informational log entry to indicate the end of a pseudo experiment.
     * 
     * Should be called after all producers have been run.
     */
    void log_event_end() {
        logtable->append(*this, database::severity::info, "end");
    }

    /** \brief Make an informational log entry to indicate the start of the run.
     * 
     * Called during pre_run.
     */
    void log_run_start() {
        logtable->append(*this, database::severity::info, "run start");
    }
    
    /** Make an informational log entry to indicate the end of the run.
     * 
     * Called during post_run.
     */
    void log_run_end() {
        logtable->append(*this, database::severity::info, "run end");
    }
private:
    //In order to implement the calling sequence, keep track of the current state with a state variable:
    //0 = just constructed
    //1 = starting to execute pre-run
    //2 = end executing pre-run
    //3 = start executing run
    //4 = end executing run
    //5 = start executing post_run
    //6 = end executing post_run
    //(having two numbers for each step makes debugging easier
    // if method execution is interrupted by exception. In this case, Run object
    // will be in an invalid state, but at least, it is detectable).
    int state;
};


}

#endif
