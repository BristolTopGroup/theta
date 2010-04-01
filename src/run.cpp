#include "interface/run.hpp"
#include "interface/histogram.hpp"

#include <signal.h>
#include <boost/date_time/local_time/local_time.hpp>

using namespace theta;
using namespace std;

bool theta::stop_execution = false;

static void sigint_handler(int sig){
   if(stop_execution) exit(1);
   stop_execution = true;
}

void theta::install_sigint_handler(){
    struct sigaction siga;
    siga.sa_handler = sigint_handler;
    siga.sa_flags = 0;
    sigemptyset(&siga.sa_mask);
    siga.sa_restorer = 0;
    sigaction(SIGINT, &siga, 0);
}

void Run::set_progress_listener(const boost::shared_ptr<ProgressListener> & l){
    progress_listener = l;
}

void Run::run(){
    //record all producers in prodinfo_table and setup the tables of the producers:
    producer_table.reset(new ProducerTable("products", db));
    for(size_t i=0; i<producers.size(); i++){
        prodinfo_table.append(static_cast<int>(i), producers[i].get_name(), producers[i].get_type(), producers[i].get_information());
        producers[i].set_table(producer_table);
        producers[i].define_table();
    }
    //write random seeds to rndinfo_table:
    rndinfo_table.append(*this, seed);
    //log the start of the run:
    eventid = 0;
    logtable->append(*this, LogTable::info, "run start");
    //call the run implementation of the subclass:
    run_impl();
    eventid = 0; // to indicate in the log table that this is an "event-wide" entry
    logtable->append(*this, LogTable::info, "run end");
    if(log_report){
        const int* n_messages = logtable->get_n_messages();
        LogTable::e_severity s = logtable->get_loglevel();
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

Run::Run(const plugin::Configuration & cfg): seed(-1), rnd(new RandomSourceTaus()),
      vm(cfg.vm), m_pseudodata(cfg.vm), m_producers(cfg.vm), db(new Database(cfg.setting["result-file"])),
      logtable(new LogTable("log", db)), log_report(true), prodinfo_table("prodinfo", db), rndinfo_table("rndinfo", db),
      runid(1), eventid(0), n_event(cfg.setting["n-events"]){
      SettingWrapper s = cfg.setting;
      if(s.exists("run-id")) runid = s["run-id"];
      int i_seed = -1;
      if(s.exists("seed")) i_seed = s["seed"];
      if(i_seed==-1){
          using namespace boost::posix_time;
          using namespace boost::gregorian;
          ptime t(microsec_clock::universal_time());
          time_duration td = t - ptime(date(1970, 1, 1));
          i_seed = td.total_microseconds();
      }
      seed = i_seed;
      rnd.set_seed(seed);
      if (s.exists("model")) {
          std::auto_ptr<Model> model = ModelFactory::buildModel(plugin::Configuration(cfg, s["model"]));
          m_pseudodata = *model;
          m_producers = *model;
      }
      if (s.exists("model-pseudodata")) {
          m_pseudodata = *ModelFactory::buildModel(plugin::Configuration(cfg, s["model-pseudodata"]));
          s["model-producers"];//will throw if not found
       }
       if (s.exists("model-producers")) {
          m_producers = *ModelFactory::buildModel(plugin::Configuration(cfg, s["model-producers"]));
          s["model-pseudodata"];//will throw if not found
       }
      params_table.reset(new ParamTable("params", db, *vm, m_pseudodata.getParameters()));
      LogTable::e_severity level = LogTable::warning;
      if(s.exists("log-level")){
         std::string loglevel = s["log-level"];
         if(loglevel=="error") level = LogTable::error;
         else if(loglevel=="warning") level = LogTable::warning;
         else if(loglevel=="info")level = LogTable::info;
         else if(loglevel=="debug")level = LogTable::debug;
         else{
             std::stringstream ss;
             ss << "log level given in " << s["log-level"].getPath() << " unknown (given '" << loglevel << "'; only allowed values are "
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
      size_t n_p = s["producers"].size();
      if (n_p == 0)
         throw ConfigurationException("no producers in run specified!");
      for (size_t i = 0; i < n_p; i++) {
        SettingWrapper producer_setting = s["producers"][i];
        std::auto_ptr<Producer> p = plugin::PluginManager<Producer>::build(plugin::Configuration(cfg, producer_setting));
        addProducer(p);
        //should transfer ownership:
        assert(p.get()==0);
      }
    }
