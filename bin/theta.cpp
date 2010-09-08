#include "interface/run.hpp"
#include "interface/cfg-utils.hpp"
#include "interface/plugin.hpp"
#include "interface/histogram.hpp"
#include "interface/variables-utils.hpp"

#include "libconfig/libconfig.h++"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <termios.h>

#include <fstream>

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

    MyProgressListener(): chars_written(0){
        if(not isatty(1)) return;
        //disable terminmal echoing; we don't expect any input.
        termios settings;
        if (tcgetattr (1, &settings) < 0) return; //ignore error
        settings.c_lflag &= ~ECHO;
        tcsetattr (1, TCSANOW, &settings);
    }
    
    ~MyProgressListener(){
        if(not isatty(1)) return;
        //enable terminmal echoing again; don't be evil
        termios settings;
        if (tcgetattr (1, &settings) < 0) return; //ignore error
        settings.c_lflag |= ECHO;
        tcsetattr (1, TCSANOW, &settings);
    }
    
private:
   int chars_written;
};

int main(int argc, char** argv) {
    po::options_description desc("Supported options");
    desc.add_options()("help,h", "show help message")
    ("quiet,q", "quiet mode (suppress progress message)")
    ("nowarn", "do not warn about unused configuration file statements");

    po::options_description hidden("Hidden options");

    hidden.add_options()
    ("cfg-file", po::value<string>(), "configuration file")
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
    bool nowarn = cmdline_vars.count("nowarn");

    Config cfg;
    boost::shared_ptr<SettingUsageRecorder> rec(new SettingUsageRecorder());
    std::auto_ptr<Run> run;
    boost::shared_ptr<VarIdManager> vm(new VarIdManager);
    
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
            s << "Error parsing configuration file: " << p.getError() << " in line " << p.getLine() << ", file " << p.getFile();
            throw ConfigurationException(s.str());
        }
        
        SettingWrapper root(cfg.getRoot(), cfg.getRoot(), rec);
        Configuration config(vm, root);
        
        //process options:
        Configuration cfg_options(config, config.setting["options"]);
        PluginLoader::execute(cfg_options);
        
        //fill VarIdManager:
        VarIdManagerUtils::apply_settings(config);
        //build run:
        run.reset(new Run(Configuration(config, root[run_name])));
        if(not quiet){
            boost::shared_ptr<ProgressListener> l(new MyProgressListener());
            run->set_progress_listener(l);
        }
    }
    catch (SettingNotFoundException & ex) {
        cerr << "Error: the required setting " << ex.getPath() << " was not found." << endl;
        return 1;
    } catch (SettingTypeException & ex) {
        cerr << "Error: the setting " << ex.getPath() << " has the wrong type." << endl;
        return 1;
    } catch (SettingException & ex) {
        cerr << "Error while processing the configuration at " << ex.getPath() << endl;
        return 1;
    } catch (Exception & e) {
        cerr << "Error: " << e.message << endl;
        return 1;
    }
    
    if(not nowarn){
        vector<string> unused;
        rec->get_unused(unused, cfg.getRoot());
        if (unused.size() > 0) {
            cout << "WARNING: following setting paths in the configuration file have not been used: " << endl;
            for (size_t i = 0; i < unused.size(); ++i) {
                cout << "  " << (i+1) << ". " << unused[i] << endl;
            }
            cout << "Comment out these settings to get rid of this message." << endl;
        }
    }

    //install signal handler now, not much earlier. Otherwise, plugin loading
    // might change it ...
    install_sigint_handler();

    try {
        run->run();
    }
    catch(ExitException & ex){
       cerr << "Exit requested: " << ex.message << endl;
       return 1;
    }
    catch (Exception & ex) {
        cerr << "An error ocurred in Run::run: " << ex.what() << endl;
        return 1;
    }
    catch(FatalException & ex){
        cerr << "A fatal error ocurred in Run::run: " << ex.message << endl;
        return 1;
    }
    if(theta::stop_execution){
        cout << "(exiting on SIGINT)" << endl;
    }
    return 0;
}
