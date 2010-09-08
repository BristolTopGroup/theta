#include "interface/run.hpp"
#include "interface/histogram.hpp"

#include <signal.h>
#include <boost/date_time/local_time/local_time.hpp>

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
    siga.sa_handler = sigint_handler;
    siga.sa_flags = 0;
    sigemptyset(&siga.sa_mask);
    siga.sa_restorer = 0;
    sigaction(SIGINT, &siga, 0);
}

void Run::set_progress_listener(const boost::shared_ptr<ProgressListener> & l){
    progress_listener = l;
}

namespace{
    template<class T>
    void noop_deleter(T*){}
}

void Run::run(){
    //make a shared_ptr out of a auto_ptr. While this is not very nice, I personally
    // like shared_ptr in interfaces much better than naked pointers as with naked pointers,
    // it is never a-priori clear who has to take care of them.
    // For a shared_ptr, the responsibility is clear.
    boost::shared_ptr<ProductsTable> products_table_shared(products_table.get(), noop_deleter<ProductsTable>);
    
    data_source->set_table(products_table_shared);
    data_source->define_table();
    for(size_t i=0; i<producers.size(); i++){
        prodinfo_table->append(static_cast<int>(i), producers[i].getName(), producers[i].getType());
        producers[i].set_table(products_table_shared);
        producers[i].define_table();
    }
    
    //log the start of the run:
    eventid = 0;
    logtable->append(*this, LogTable::info, "run start");
    
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

void Run::addProducer(std::auto_ptr<Producer> & p){
    producers.push_back(p);
}

Run::Run(const plugin::Configuration & cfg): rnd(new RandomSourceTaus()),
  vm(cfg.vm), /*db(new Database(cfg.setting["result-file"])),  logtable(new LogTable("log", db)),*/
  log_report(true), /*, prodinfo_table("prodinfo", db), rndinfo_table("rndinfo", db),*/
      runid(1), eventid(0), n_event(cfg.setting["n-events"]){
    SettingWrapper s = cfg.setting;
    
    //1. setup database and tables:
    db = plugin::PluginManager<Database>::build(plugin::Configuration(cfg, s["output_database"]));

    std::auto_ptr<Table> prodinfo_table_underlying = db->create_table("prodinfo");
    prodinfo_table.reset(new ProducerInfoTable(prodinfo_table_underlying));
    
    std::auto_ptr<Table> logtable_underlying = db->create_table("log");
    logtable.reset(new LogTable(logtable_underlying));
    
    std::auto_ptr<Table> rndinfo_table_underlying = db->create_table("rndinfo");
    rndinfo_table.reset(new RndInfoTable(rndinfo_table_underlying));
    
    std::auto_ptr<Table> products_table_underlying = db->create_table("products");
    products_table.reset(new ProductsTable(products_table_underlying));
    
    //2. misc
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
    //get the runid from the rndinfo table:
    runid = rndinfo_table->append(seed);
    
    model = ModelFactory::buildModel(plugin::Configuration(cfg, s["model"]));
    if(s.exists("data_source"))
        data_source = plugin::PluginManager<DataSource>::build(plugin::Configuration(cfg, s["data_source"]));
    else
        data_source = plugin::PluginManager<DataSource>::build(plugin::Configuration(cfg, s["data-source"]));
    
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
    eventid = 0;
    
    //4. add the producers:
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
