#include "test/utils.hpp"

#include "interface/plugin.hpp"

#include <string>
#include <boost/test/unit_test.hpp>

using namespace theta;
using namespace theta::plugin;


ConfigCreator::ConfigCreator(const std::string & cfg_string, const boost::shared_ptr<theta::VarIdManager> & vm):
      dummy(setup_config(cfg_string)), cfg(vm, SettingWrapper(config.getRoot(), config.getRoot(), rec)){
}

int ConfigCreator::setup_config(const std::string & cfg_string){
    config.readString(cfg_string);
    return 0;
}


void load_core_plugins(){
    static bool loaded(false);
    if(loaded) return;
    BOOST_TEST_CHECKPOINT("loading core plugin");
    try{
        PluginLoader::load("lib/core-plugins.so");
    }
    catch(Exception & ex){
      std::cout << ex.message << std::endl;
      throw;
    }
    BOOST_TEST_CHECKPOINT("loaded core plugin");
    loaded = true;
}
