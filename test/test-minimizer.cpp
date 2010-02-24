#include "interface/plugin.hpp"
#include "interface/cfg-utils.hpp"

#include <boost/test/unit_test.hpp> 

using namespace std;
using namespace theta;
using namespace libconfig;
using namespace theta::plugin;
using namespace theta::utils;

BOOST_AUTO_TEST_SUITE(minimizer_tests)

static bool root_plugin = false;

class ImpossibleFunction: public Function{
public:
    ImpossibleFunction(const ParIds & pids):Function(pids){
    }

    virtual double operator()(const ParValues & v) const{
        return 0.0;
    }

    virtual double gradient(const ParValues & v, const ParId & pid) const{
        return 0.0;
    }
};

BOOST_AUTO_TEST_CASE(minuit){
    Config cfg;
    Setting & s = cfg.getRoot();
    s.add("files", Setting::TypeList);
    s["files"].add(Setting::TypeString);
    s["files"][0] = "lib/root.so";
    SettingUsageRecorder rec;
    try{
       PluginLoader::execute(s, rec);
    }
    catch(Exception & ex){
      cout << "Note: root plugin could not be loaded, not executing root tests";
      return;
    }
    root_plugin = true;
    
    s.add("min", Setting::TypeGroup);
    s["min"].add("type", Setting::TypeString);
    s["min"]["type"] = "root-minuit";
    
    boost::shared_ptr<VarIdManager> vm(new VarIdManager);
    ParId p0 = vm->createParId("p0");
    ParId p1 = vm->createParId("p1");
    ParIds pars;
    pars.insert(p0);
    pars.insert(p1);
    
    ConfigurationContext ctx(vm, s, s["min"], rec);
    std::auto_ptr<Minimizer> min = PluginManager<MinimizerFactory>::get_instance()->build(ctx);
    BOOST_REQUIRE(min.get());
    ImpossibleFunction f(pars);
    bool exception;
    try{
        BOOST_REQUIRE(true);//create checkpoint
        min->minimize(f);
    }
    catch(MinimizationException & ex){
        exception = true;
    }
    catch(std::exception & ex){ //should not happen ...
        cout << ex.what() << endl;
        BOOST_REQUIRE(false);
    }
    BOOST_REQUIRE(exception);
}

BOOST_AUTO_TEST_SUITE_END()

