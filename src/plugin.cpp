#include "interface/plugin.hpp"
#include "interface/histogram.hpp"
#include "interface/distribution.hpp"
#include "interface/phys.hpp"
#include "interface/histogram-function.hpp"
#include "interface/run.hpp"
#include "interface/minimizer.hpp"

using namespace theta::plugin;
using namespace std;

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

ProductsTableWriter::~ProductsTableWriter(){}

ProductsTableWriter::ProductsTableWriter(const Configuration & cfg){
    type = static_cast<string>(cfg.setting["type"]);
    name = static_cast<string>(cfg.setting["name"]);
    if(not nameOk(name)){
        throw InvalidArgumentException("name '" + name + "' is not a valid name for building column names. ");
    }
}

//on gcc 4.4.1, commenting out this unused function will break things: some plugin types are not found any more (here: root_minuit, which
// provides a Minimizer). I have *no* idea why this is so. Maybe because the print_registered_plugins is forcing an instantiation of
// PluginManager<...>::factories in this object file?
namespace{
    void print_registered_types(){
        std::vector<std::string> names = PluginManager<theta::HistogramFunction>::get_registered_types();
        cout << "HistogramFunctions: ";
        for(size_t i=0; i<names.size(); ++i){
            cout << names[i] << " ";
        }
        cout << endl;
        
        names = PluginManager<theta::Minimizer>::get_registered_types();
        cout << "Minimizers: ";
        for(size_t i=0; i<names.size(); ++i){
            cout << names[i] << " ";
        }
        cout << endl;
    }
    
}

void PluginLoader::execute(const Configuration & cfg) {
    SettingWrapper files = cfg.setting["plugin_files"];
    size_t n = files.size();
    for (size_t i = 0; i < n; i++) {
        std::string filename = files[i];
        load(filename);
    }
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
    /*if(false)
        print_registered_types();*/
}
