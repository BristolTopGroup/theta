#include "interface/plugin_so_interface.hpp"
#include "interface/run.hpp"
#include "interface/cfg-utils.hpp"
#include "interface/plugin.hpp"
#include "interface/histogram.hpp"
#include "interface/variables-utils.hpp"

#include "libconfig/libconfig.h++"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

using namespace std;
using namespace theta;
using namespace theta::utils;
using namespace theta::plugin;
using namespace libconfig;

namespace fs = boost::filesystem;
namespace po = boost::program_options;

class MyProgressListener: public ProgressListener{
public:
    virtual void progress(int done, int total){
      if(isatty(1)){
          //move back to beginning of terminal line:
          printf("\033[%dD", chars_written);
          chars_written = 0;
          double d = 100.0 * done / total;
          chars_written += printf("%6d / %-6d [%5.1f%%] ", done, total, d);
          cout.flush();
        }
    }

    MyProgressListener(): chars_written(0){}
private:
   int chars_written;
};

int main(int argc, char** argv) {
    po::options_description desc("Supported options");
    desc.add_options()("help", "show help message")
    ("quiet,q", "quiet mode (supress progress message)");

    po::options_description hidden("Hidden options");

    hidden.add_options()
    ("cfg-file", po::value<string>(), "config file")
    ("run-name", po::value<string>(), "run name");

    po::positional_options_description p;
    p.add("cfg-file", 1);
    p.add("run-name", 1);

    po::options_description cmdline_options;
    cmdline_options.add(desc).add(hidden);

    po::variables_map cmdline_vars;
    try{
        po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), cmdline_vars);
    }
    catch(std::exception & ex){
        cerr << "Error parsing command line options: " << ex.what() << endl;
        return 1;
    }
    po::notify(cmdline_vars);
    
    if(cmdline_vars.count("help")){
        cout << desc << endl;
        return 0;
    }
    
    if(cmdline_vars.count("cfg-file")==0){
        cerr << "Error: you have to specify a configuration file" << endl;
        return 1;
    }
    
    string cfg_filename = cmdline_vars["cfg-file"].as<string>();
    string run_name = "main";
    if(cmdline_vars.count("run-name")) run_name = cmdline_vars["run-name"].as<string>();
    bool quiet = cmdline_vars.count("quiet");
    

    Config cfg;
    
    std::auto_ptr<Run> run;
    boost::shared_ptr<VarIdManager> vm(new VarIdManager);
    SettingUsageRecorder rec;

    try {
        try {
            //as includes in config files shouled always be resolved relative to the config file's location:
            string old_path = fs::current_path().string();
            //convert any failure to a FileIOException:
            try{
                 if(fs::path(cfg_filename).has_parent_path()){
                    fs::current_path(fs::path(cfg_filename).parent_path());
                 }
                 cfg_filename = fs::path(cfg_filename).filename();
            }
            catch(fs::filesystem_error & ex){
                 throw FileIOException();
            }
            cfg.readFile(cfg_filename.c_str());
            fs::current_path(old_path);
        } catch (FileIOException & f) {
            stringstream s;
            s << "Configuration file " << cfg_filename << " could not be read";
            throw ConfigurationException(s.str());
        } catch (ParseException & p) {
            stringstream s;
            s << "Error parsing the given configuration file: " << p.getError() << " in line " << p.getLine();
            throw ConfigurationException(s.str());
        }
        SettingWrapper root(cfg.getRoot(), cfg.getRoot(), rec);
        Configuration config(vm, root);
        
        //load plugins:
        if(root.exists("plugins")){
            PluginLoader::execute(Configuration(config, root["plugins"]));
        }
        //fill VarIdManager:
        VarIdManagerUtils::apply_settings(config);
        //build run:
        run = PluginManager<Run>::build(Configuration(config, root[run_name]));
        if(not quiet){
            boost::shared_ptr<ProgressListener> l(new MyProgressListener());
            run->set_progress_listener(l);
        }
    } catch (ConfigurationException & ex) {
        cerr << "Error while building Model from config: " << ex.message << "." << endl;
    } catch (SettingNotFoundException & ex) {
        cerr << "The required configuration parameter '" << ex.getPath() << "' was not found." << endl;
    } catch (SettingTypeException & ex) {
        cerr << "The configuration parameter " << ex.getPath() << " has the wrong type." << endl;
    } catch (SettingException & ex) {
        cerr << "An unspecified setting exception while processing the configuration occured in path " << ex.getPath() << endl;
    } catch (Exception & e) {
        cerr << "An exception occured while processing the configuration: " << e.message << endl;
    }

    if (run.get() == 0) {
        cerr << "Could not build run object due to occurred errors. Exiting." << endl;
        return 1;
    }
    
    vector<string> unused;
    rec.get_unused(unused, cfg.getRoot());
    if (unused.size() > 0) {
        cout << "WARNING: following setting paths in the configuration file have not been used: " << endl;
        for (size_t i = 0; i < unused.size(); ++i) {
            cout << "  " << (i+1) << ". " << unused[i] << endl;
        }
        cout << "Comment out these settings to get rid of this message." << endl;
        cout << "(NOTE: this warning feature is still experimental. If false positives were reported, please write a ticket.)" << endl << endl;
    }

    //install signal handler now, not much earlier. Otherwise, plugin loading
    // might change it ...
    install_sigint_handler();

    try {
        run->run();
    } catch (Exception & ex) {
        cerr << "An exception ocurred: " << ex.what() << endl;
        return 1;
    }
    if(theta::stop_execution){
        cout << "(exiting on SIGINT)" << endl;
    }
    return 0;
}
