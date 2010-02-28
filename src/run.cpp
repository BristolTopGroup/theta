#include "interface/run.hpp"
#include "interface/histogram.hpp"

using namespace theta;
using namespace std;

void Run::set_progress_listener(const boost::shared_ptr<ProgressListener> & l){
    progress_listener = l;
}

void Run::run(){
    //record all producers in prodinfo_table:
    for(size_t i=0; i<producers.size(); i++){
        prodinfo_table.append(*this, (int)i, producers[i].get_name(), producers[i].get_type());
    }
    //write random seeds to rndinfo_table:
    rndinfo_table.append(*this, seed);
    eventid = 0;
    logtable->append(*this, database::severity::info, "run start");
    //call the run implementation of the subclass:
    run_impl();
    eventid = 0; // to indicate in the log table that this is an "event-wide" entry
    logtable->append(*this, database::severity::info, "run end");
    if(log_report){
        const int* n_messages = logtable->get_n_messages();
        database::severity::e_severity s = logtable->get_loglevel();
        cout << endl << endl << "Log report:" << endl;
        cout << "  errors:   " << setw(6) << n_messages[0] << endl;
        if(s > 0)
            cout << "  warnings: " << setw(6) << n_messages[1] << endl;
        if(s > 1)
            cout << "  infos:    " << setw(6) << n_messages[2] << endl;
        if(s > 2)
            cout << "  debug:    " << setw(6) << n_messages[3] << endl;
    }
}

void Run::addProducer(std::auto_ptr<Producer> & p){
    producers.push_back(p);
}

Run::Run(plugin::Configuration & cfg): seed(-1), rnd(new RandomSourceTaus()),
      vm(cfg.vm), m_pseudodata(cfg.vm), m_producers(cfg.vm), db(new database::Database(cfg.setting["result-file"])),
      logtable(new database::LogTable("log")), log_report(true), prodinfo_table("prodinfo"), rndinfo_table("rndinfo"),
      runid(1), eventid(0), n_event(cfg.setting["n-events"]){
          
      const libconfig::Setting & s = cfg.setting;
      if(s.lookupValue(static_cast<const char*>("run-id"), runid)){
         cfg.rec.markAsUsed(s["run-id"]);
      }
      int i_seed = -1;
      if(s.lookupValue(static_cast<const char*>("seed"), i_seed)){
         cfg.rec.markAsUsed(s["seed"]);
      }
      if(i_seed==-1) i_seed = time(0);
      seed = i_seed;
      rnd.set_seed(seed);
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
      params_table.reset(new database::ParamTable("params", *vm, m_pseudodata.getParameters()));
      params_table->connect(db);
      database::severity::e_severity level = database::severity::warning;
      if(s.exists("log-level")){
         std::string loglevel = s["log-level"];
         if(loglevel=="error") level = database::severity::error;
         else if(loglevel=="warning") level = database::severity::warning;
         else if(loglevel=="info")level = database::severity::info;
         else if(loglevel=="debug")level = database::severity::debug;
         else{
             std::stringstream ss;
             ss << "log level given in " << s["log-level"].getPath() << " on line "
                << s["log-level"].getSourceLine() << " unknown (given '" << loglevel << "'; only allowed values are "
                << "'error', 'warning', 'info' and 'debug')";
            throw ConfigurationException(ss.str());
         }
      }
      logtable->set_loglevel(level);
      if(s.exists("log-report")){
          log_report = s["log-report"];
      }
      
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
