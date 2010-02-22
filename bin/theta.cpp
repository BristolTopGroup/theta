#include "interface/run.hpp"
#include "interface/cfg-utils.hpp"
#include "interface/plugin.hpp"
#include "libconfig/libconfig.h++"

#include <boost/timer.hpp>

using namespace std;
using namespace theta;
using namespace theta::utils;
using namespace theta::plugin;
using namespace libconfig;

int main(int argc, char** argv) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <configuration file> <run name>" << endl;
        exit(1);
    }

    Config cfg;
    
    std::auto_ptr<Run> run;
    Setting * proot = 0;
    boost::shared_ptr<VarIdManager> vm(new VarIdManager);
    SettingUsageRecorder rec;

    try {
        try {
            cfg.readFile(argv[1]);
        } catch (FileIOException & f) {
            stringstream s;
            s << "Configuration file " << argv[1] << " could not be read.";
            throw ConfigurationException(s.str());
        } catch (ParseException & p) {
            stringstream s;
            s << "Error parsing the given configuration file: " << p.getError() << " in line " << p.getLine();
            throw ConfigurationException(s.str());
        }
        Setting & root = cfg.getRoot();
        proot = &root;
        if(root.exists("plugins")){
            PluginLoader::execute(root["plugins"], rec);
            //rec.remove_used(root);
        }
        Setting & runsetting = root[(const char*) argv[2]];
        ConfigurationContext run_context(vm, root, runsetting, rec);

        run = PluginManager<RunFactory>::get_instance()->build(run_context);
        //std::auto_ptr<Model> model = ExceptionWrapper<std::auto_ptr<Model> >(boost::bind<std::auto_ptr<Model> >(&ModelFactory::buildModel, modelsetting, vm));
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
        cout << "CONFIG WARNING: following paths in the configuration file were IGNORED: " << endl;
        for (size_t i = 0; i < unused.size(); i++) {
            cout << unused[i] << endl;
        }
        cout << "(end CONFIG WARNING)" << endl << endl;
    }

    try {
        run->pre_run();
        run->run();
        run->post_run();
    } catch (Exception & ex) {
        cerr << "An exception ocurred: " << ex.what() << endl;
        return 1;
    }

    return 0;
}
