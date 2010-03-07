#include "interface/exception.hpp"
#include "interface/plugin.hpp"

#include "interface/variables.hpp"
#include "interface/phys.hpp"

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
    boost::shared_ptr<VarIdManager> vm(new VarIdManager);
    Configuration ctx(vm, SettingWrapper(s, s, rec));
    try{
        PluginLoader::execute(ctx);
    }
    catch(Exception & ex){
        BOOST_REQUIRE_EQUAL(ex.message, "");
        BOOST_REQUIRE(false);
    }    
    //test-ex.so defines a Function named "test-exception":
    s.add("type", Setting::TypeString);
    s["type"] = "test_exception";
    BOOST_REQUIRE(true); // create checkpoint
    std::auto_ptr<Function> f;
    try{
         f = PluginManager<Function>::build(ctx);
    }
    catch(Exception & ex){
        BOOST_REQUIRE_EQUAL(ex.message, "");
        BOOST_REQUIRE(false);
    }
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

//depends on etest_plugin being run before to load the plugin
// tests whether exceptions occuring during loading a new plugin (i.e., not during execution)
// are correctly caught
BOOST_AUTO_TEST_CASE(etest_plugin_build) {
    Config cfg;
    SettingUsageRecorder rec;
    Setting & s = cfg.getRoot();
    s.add("type", Setting::TypeString);
    s["type"] = "test_ex_during_build";
    boost::shared_ptr<VarIdManager> vm(new VarIdManager);
    Configuration ctx(vm, SettingWrapper(s, s, rec));
    BOOST_REQUIRE(true); // create checkpoint
    bool exception = false;
    try{
        PluginManager<Function>::build(ctx);
    }
    catch(Exception & ex){
       //does not have to be equal, but original message must appear somewhere:
       BOOST_REQUIRE(ex.message.find("exception message 23")!=string::npos);
       exception = true;
    }
    BOOST_REQUIRE(exception);
}

//tests whether exceptions thrown by plugins are caught by plugins:
// let the test-exception2 build a test-exception object and "proxy" to it.
BOOST_AUTO_TEST_CASE(etest_plugin_plugin) {
    Config cfg;
    SettingUsageRecorder rec;
    Setting & s = cfg.getRoot();
    s.add("type", Setting::TypeString);
    s["type"] = "proxy_function";
    s.add("block", Setting::TypeGroup);
    s["block"].add("type", Setting::TypeString);
    s["block"]["type"] = "test_exception";
    boost::shared_ptr<VarIdManager> vm(new VarIdManager);
    Configuration ctx(vm, SettingWrapper(s, s, rec));
    BOOST_REQUIRE(true); // create checkpoint
    auto_ptr<Function> proxy_function = PluginManager<Function>::build(ctx);
    
    ParValues values;
    bool exception = false;
    try{
       (*proxy_function)(values);
    }
    catch(Exception & ex){
       BOOST_REQUIRE_EQUAL(ex.message, "exception caught by proxy_function: test-exception message 42");
       exception = true;
    }
    BOOST_REQUIRE(exception);
}


BOOST_AUTO_TEST_SUITE_END()

