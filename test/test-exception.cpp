#include "interface/exception.hpp"
#include "interface/plugin.hpp"

//#include "interface/plugin.hpp"

#include <boost/test/unit_test.hpp>

using namespace std;
using namespace theta;
using namespace theta::plugin;
using namespace theta::utils;
using namespace libconfig;

BOOST_AUTO_TEST_SUITE(exception_tests)

//This test could fail with a segmentation violation if linking libtheta.so without "-rdynamic" (="-Wl,-E").
BOOST_AUTO_TEST_CASE(etest){
    try{
        throw Exception("testex");
    }
    catch(Exception & ex){
        BOOST_REQUIRE(ex.message == string("testex"));
    }
}

//more sophisticated: load external .so file vis the plugin system
// and catch an exception created there ...
BOOST_AUTO_TEST_CASE(etest_plugin) {
    Config cfg;
    Setting & s = cfg.getRoot();
    s.add("files", Setting::TypeList);
    s["files"].add(Setting::TypeString);
    s["files"][0] = "test/test-ex.so";
    SettingUsageRecorder rec;
    PluginLoader::execute(s, rec);
    //test-ex.so defines a Function named "test-exception":
    s.add("type", Setting::TypeString);
    s["type"] = "test-exception";
    boost::shared_ptr<VarIdManager> vm(new VarIdManager);
    ConfigurationContext ctx(vm, s, s, rec);
    BOOST_REQUIRE(true); // create checkpoint
    std::auto_ptr<Function> f = PluginManager<FunctionFactory>::get_instance()->build(ctx);
    ParValues values;
    bool exception = false;
    try{
       BOOST_REQUIRE(true); // create checkpoint
       (*f)(values);
    }
    catch(Exception & ex){
       BOOST_REQUIRE(ex.message == "test-exception message 42");
       exception = true;
    }
    BOOST_REQUIRE(exception);
}


BOOST_AUTO_TEST_SUITE_END()
