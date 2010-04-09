#include "interface/plugin.hpp"
#include "interface/histogram.hpp"
#include "interface/distribution.hpp"
#include "interface/phys.hpp"
#include "interface/histogram-function.hpp"
#include "interface/run.hpp"
#include "interface/minimizer.hpp"

using namespace theta::plugin;

namespace{
bool nameOk(const std::string & name){
    if(name=="") return false;
    for(size_t i=0; i<name.size(); ++i){
        char c = name[i];
        if((c >= 'A') && (c <= 'Z')) continue;
        if((c >= 'a') && (c <= 'z')) continue;
        if((c >= '0') && (c <= '9')) continue;
        if(c=='_') continue;
        return false;
    }
    return true;
}

}

EventTableWriter::EventTableWriter(const std::string & name){
    if(not nameOk(name)){
        std::cerr << "Warning: name '" << name << "' is not a valid name for building column names. "
                 "Support for such names will be removed soon (set another name by chaning the right hand "
                 "side of the setting or with the name=\"...\" setting)." << std::endl;
    }
}

PluginType::PluginType(const Configuration & c): name(c.setting.getName()), type(c.setting["type"]){
    if(c.setting.exists("name")){
        name = static_cast<std::string>(c.setting["name"]);
    }
}

void PluginLoader::execute(const Configuration & cfg) {
    bool verbose = false;
    if (cfg.setting.exists("verbose")) {
        verbose = cfg.setting["verbose"];
    }
    size_t n = cfg.setting["files"].size();
    SettingWrapper files = cfg.setting["files"];
    for (size_t i = 0; i < n; i++) {
        std::string filename = files[i];
        load(filename);
    }
    if (verbose) {
        std::cout << "Known plugins:" << std::endl;
        print_plugins();
    }
}

void PluginLoader::print_plugins() {
    std::cout << "  HistogramFunctions: ";
    std::vector<std::string> typenames = PluginManager<HistogramFunction>::get_registered_types();
    for (size_t i = 0; i < typenames.size(); ++i) {
        std::cout << typenames[i] << " ";
    }
    std::cout << std::endl;

    std::cout << "  Functions: ";
    typenames = PluginManager<Function>::get_registered_types();
    for (size_t i = 0; i < typenames.size(); ++i) {
        std::cout << typenames[i] << " ";
    }
    std::cout << std::endl;

/*    std::cout << "  Runs: ";
    typenames = PluginManager<Run>::get_registered_types();
    for (size_t i = 0; i < typenames.size(); ++i) {
        std::cout << typenames[i] << " ";
    }
    std::cout << std::endl;*/

    std::cout << "  Minimizers: ";
    typenames = PluginManager<Minimizer>::get_registered_types();
    for (size_t i = 0; i < typenames.size(); ++i) {
        std::cout << typenames[i] << " ";
    }
    std::cout << std::endl;

    std::cout << "  Producers: ";
    typenames = PluginManager<Producer>::get_registered_types();
    for (size_t i = 0; i < typenames.size(); ++i) {
        std::cout << typenames[i] << " ";
    }
    std::cout << std::endl;

    std::cout << "  Distributions: ";
    typenames = PluginManager<Distribution>::get_registered_types();
    for (size_t i = 0; i < typenames.size(); ++i) {
        std::cout << typenames[i] << " ";
    }
    std::cout << std::endl;
}

void PluginLoader::load(const std::string & soname) {
    void* handle = 0;
    try {
        handle = dlopen(soname.c_str(), RTLD_NOW);
    } catch (Exception & ex) {
        std::stringstream ss;
        ss << ex.message << " (in PluginLoader::load while loading plugin file '" << soname << "')";
        ex.message = ss.str();
        throw;
    }
    if (handle == 0) {
        std::stringstream s;
        const char * error = dlerror();
        if (error == 0) error = "0";
        s << "PluginLoader::load: error loading plugin file '" << soname << "': " << error << std::endl;
        throw InvalidArgumentException(s.str());
    }
}
