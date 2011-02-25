#include "interface/run.hpp"
#include "interface/histogram.hpp"

#include <signal.h>
#include <iomanip>


using namespace theta;
using namespace std;

bool theta::stop_execution = false;

static void sigint_handler(int sig){
   if(stop_execution){
      throw ExitException("user insisted with several SIGINT");
   }
   stop_execution = true;
}

void theta::install_sigint_handler(){
    struct sigaction siga;
    memset(&siga, 0, sizeof(struct sigaction));
    siga.sa_handler = sigint_handler;
    sigemptyset(&siga.sa_mask);
    sigaction(SIGINT, &siga, 0);
}

void Run::set_progress_listener(const boost::shared_ptr<ProgressListener> & l){
    progress_listener = l;
}

void Run::run(){
    //log the start of the run:
    eventid = 0;
    logtable->append(*this, LogTable::info, "run start");
   
    Data data;
    //main event loop:
    for (eventid = 1; eventid <= n_event; eventid++) {
        if(stop_execution)break;
        try{
            data_source->fill(data, *this);
        }
        catch(DataSource::DataUnavailable &){
            break;
        }
        log_event_start();
        bool error = false;
        for (size_t j = 0; j < producers.size(); j++) {
            try {
                producers[j].produce(*this, data, *model);
            } catch (Exception & ex) {
                error = true;
                std::stringstream ss;
                ss << "Producer '" << producers[j].getName() << "' failed: " << ex.message << ".";
                logtable->append(*this, LogTable::error, ss.str());
            }
            catch(FatalException & f){
                stringstream ss;
                ss << "Producer '" << producers[j].getName() << "': " << f.message;
                f.message = ss.str();
                throw;
            }
        }
        //only add a row if no error ocurred to prevent NULL values and similar things ...
        if(!error){
            products_table->add_row(*this);
        }
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

void Run::init(const plugin::Configuration & cfg){
    SettingWrapper s = cfg.setting;
    
    //0. set default values for members:
    vm = cfg.vm;
    log_report = true;
    runid = 1;
    eventid = 0;
    n_event = s["n-events"];
    
    //1. setup database and tables:
    db = plugin::PluginManager<Database>::instance().build(plugin::Configuration(cfg, s["output_database"]));

    std::auto_ptr<Table> logtable_underlying = db->create_table("log");
    logtable.reset(new LogTable(logtable_underlying));
    
    std::auto_ptr<Table> rndinfo_table_underlying = db->create_table("rndinfo");
    rndinfo_table.reset(new RndInfoTable(rndinfo_table_underlying));
    
    std::auto_ptr<Table> products_table_underlying = db->create_table("products");
    products_table.reset(new ProductsTable(products_table_underlying));
    
    //2. model and data_source
    model = plugin::PluginManager<Model>::instance().build(plugin::Configuration(cfg, s["model"]));
    data_source = plugin::PluginManager<DataSource>::instance().build(plugin::Configuration(cfg, s["data_source"]));
    
    //3. logging stuff
    LogTable::e_severity level = LogTable::warning;
    if(s.exists("log-level")){
        std::string loglevel = s["log-level"];
        if(loglevel=="error") level = LogTable::error;
        else if(loglevel=="warning") level = LogTable::warning;
        else if(loglevel=="info")level = LogTable::info;
        else if(loglevel=="debug")level = LogTable::debug;
        else{
            std::stringstream ss;
            ss << "log level given in " << s["log-level"].getPath() << " unknown (given '" << loglevel << 
                  "'; only allowed values are 'error', 'warning', 'info' and 'debug')";
            throw ConfigurationException(ss.str());
        }
    }
    logtable->set_loglevel(level);
    if(s.exists("log-report")){
        log_report = s["log-report"];
    }
    
    //4. producers:
    size_t n_p = s["producers"].size();
    if (n_p == 0)
        throw ConfigurationException("no producers in run specified!");
    for (size_t i = 0; i < n_p; i++) {
        SettingWrapper producer_setting = s["producers"][i];
        std::auto_ptr<Producer> p = plugin::PluginManager<Producer>::instance().build(plugin::Configuration(cfg, producer_setting));
        producers.push_back(p);
    }
}
