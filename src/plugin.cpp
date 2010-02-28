#include "interface/plugin.hpp"
#include "interface/histogram.hpp"
#include "interface/distribution.hpp"
#include "interface/phys.hpp"
#include "interface/histogram-function.hpp"
#include "interface/run.hpp"
#include "interface/minimizer.hpp"

using namespace theta::plugin;

void PluginLoader::execute(const libconfig::Setting & s, theta::utils::SettingUsageRecorder & rec) {
    bool verbose = false;
    if (s.exists("verbose")) {
        verbose = s["verbose"];
        rec.markAsUsed(s["verbose"]);
    }
    int n = s["files"].getLength();
    const libconfig::Setting & files = s["files"];
    for (int i = 0; i < n; i++) {
        std::string filename = files[i];
        load(filename);
    }
    rec.markAsUsed(s["files"]);
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

    std::cout << "  Runs: ";
    typenames = PluginManager<Run>::get_registered_types();
    for (size_t i = 0; i < typenames.size(); ++i) {
        std::cout << typenames[i] << " ";
    }
    std::cout << std::endl;

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

std::vector<std::string> PluginLoader::loaded_files;

void PluginLoader::load(const std::string & soname) {
    if(find(loaded_files.begin(), loaded_files.end(), soname)!=loaded_files.end()) return;
    void* handle = 0;
    try {
        handle = dlopen(soname.c_str(), RTLD_NOW);
    } catch (Exception & ex) {
        std::cerr << "Exception while loading plugin file '" << soname << "': " << ex.message << std::endl;
    }
    if (handle == 0) {
        std::stringstream s;
        const char * error = dlerror();
        if (error == 0) error = "0";
        s << "PluginLoader::load: error loading plugin file '" << soname << "': " << error << std::endl;
        throw InvalidArgumentException(s.str());
    }
    loaded_files.push_back(soname);
}
