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
    event_table.reset(new EventTable("events", db));
    data_source->set_table(event_table);
    data_source->define_table();
    for(size_t i=0; i<producers.size(); i++){
        prodinfo_table.append(static_cast<int>(i), producers[i].get_name(), producers[i].get_type(),
                              producers[i].get_information());
        producers[i].set_table(event_table);
        producers[i].define_table();
    }
    //write random seeds to rndinfo_table:
    //TODO: ...
    rndinfo_table.append(*this, seed);
    //log the start of the run:
    eventid = 0;
    logtable->append(*this, LogTable::info, "run start");
    
    //main event loop:
    for (eventid = 1; eventid <= n_event; eventid++) {
        if(stop_execution)break;
        try{
            data_source->fill(data);
        }
        catch(DataSource::DataUnavailable &){
            break;
        }
        log_event_start();
        //ParValues values = m_pseudodata.sampleValues(rnd);
        //data = m_pseudodata.samplePseudoData(rnd, values);
        //params_table->append(*this, values);
        for (size_t j = 0; j < producers.size(); j++) {
            try {
                producers[j].produce(*this, data, *m_producers);
            } catch (Exception & ex) {
                std::stringstream ss;
                ss << "Producer '" << producers[j].get_name() << "' failed: " << ex.message;
                logtable->append(*this, LogTable::error, ss.str());
            }
        }
        event_table->add_row(*this);
        log_event_end();
        if(progress_listener) progress_listener->progress(eventid, n_event);
    }
    
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

Run::Run(const plugin::Configuration & cfg): rnd(new RandomSourceTaus()), vm(cfg.vm), db(new Database(cfg.setting["result-file"])),
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
      m_producers = ModelFactory::buildModel(plugin::Configuration(cfg, s["model"]));
      data_source = plugin::PluginManager<DataSource>::build(plugin::Configuration(cfg, s["data-source"]));
      
      //params_table.reset(new ParamTable("params", db, *vm, m_pseudodata.getParameters()));
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
