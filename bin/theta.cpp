#include "interface/plugin_so_interface.hpp"
#include "interface/run.hpp"
#include "interface/cfg-utils.hpp"
#include "interface/plugin.hpp"
#include "interface/variables-utils.hpp"

#include "libconfig/libconfig.h++"

#include <boost/timer.hpp>
#include <boost/filesystem.hpp>

using namespace std;
using namespace theta;
using namespace theta::utils;
using namespace theta::plugin;
using namespace libconfig;

namespace fs = boost::filesystem;

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
    if (argc < 2 || strcmp("-h", argv[1])==0 || strcmp("--help", argv[1])==0) {
        cerr << "Usage: " << argv[0] << " <configuration file> [run name]" << endl;
        cerr << "If no run name is given, 'main' is assumed." << endl;
        exit(1);
    }
    string run_name;
    if(argc == 2){
       run_name = "main";
    }
    else{
       run_name = argv[2];
    }
    string cfg_filename = argv[1];

    Config cfg;
    
    std::auto_ptr<Run> run;
    Setting * proot = 0;
    boost::shared_ptr<VarIdManager> vm(new VarIdManager);
    SettingUsageRecorder rec;

    try {
        try {
            //as includes in config files shouled always be resolved relative to the config file's location:
            string old_path = fs::current_path().string();
            //convert any failure to a FileIOException:
            try{
                 fs::current_path(fs::path(cfg_filename).parent_path());
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
        Setting & root = cfg.getRoot();
        proot = &root;
        
        //load plugins:
        if(root.exists("plugins")){
            PluginLoader::execute(root["plugins"], rec);
        }
        //fill VarIdManager:
        Configuration vm_context(vm, root, root, rec);
        VarIdManagerUtils::apply_settings(vm_context);
        //build run:
        Setting & runsetting = root[run_name];
        Configuration run_context(vm_context, runsetting);
        run = PluginManager<Run>::build(run_context);
        boost::shared_ptr<ProgressListener> l(new MyProgressListener());
        run->set_progress_listener(l);
    } catch (ConfigurationException & ex) {
        cerr << "Error while building Model from config: " << ex.message << "." << endl;
    } catch (SettingNotFoundException & ex) {
        cerr << "The required configuration parameter " << ex.getPath() << " was not found." << endl;
    } catch (SettingTypeException & ex) {
        cerr << "The configuration parameter " << ex.getPath() << " has the wrong type." << endl;
    } catch (SettingException & ex) {
        cerr << "An unspecified setting exception while processing the configuration occured in path " << ex.getPath() << endl;
    } catch (Exception & e) {
        cerr << "An exception occured while processing the configuration: " << e.message << endl;
    }

    if (run.get() == 0 || proot==0) {
        cerr << "Could not build run object due to occurred errors. Exiting." << endl;
        return 1;
    }
    
    Setting & root = *proot;
    rec.remove_used(root);
    remove_empty_groups(root);
    vector<string> unused = get_paths(root);
    if (unused.size() > 0) {
        cout << "WARNING: following setting paths in the configuration file have not been used: " << endl;
        for (size_t i = 0; i < unused.size(); ++i) {
            cout << "  " << (i+1) << ". " << unused[i] << endl;
        }
        cout << "Comment out these settings to get rid of this message." << endl << endl;
    }

    try {
        run->pre_run();
        run->run();
        run->post_run();
    } catch (Exception & ex) {
        cerr << "An exception ocurred: " << ex.what() << endl;
        return 1;
    }
    cout << endl;

    return 0;
}
