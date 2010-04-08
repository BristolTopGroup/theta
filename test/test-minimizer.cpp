#include "interface/plugin.hpp"
#include "interface/cfg-utils.hpp"
#include "interface/phys.hpp"
#include "interface/minimizer.hpp"

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
    ImpossibleFunction(const ParIds & pids){
        par_ids = pids;
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
    BOOST_REQUIRE(true);//create checkpoint
    Setting & s = cfg.getRoot();
    s.add("files", Setting::TypeList);
    s["files"].add(Setting::TypeString);
    s["files"][0] = "lib/root.so";
    SettingUsageRecorder rec;
    boost::shared_ptr<VarIdManager> vm(new VarIdManager);
    try{
       PluginLoader::execute(Configuration(vm, SettingWrapper(s, s, rec)));
    }
    catch(Exception & ex){
      cout << "Note: root plugin could not be loaded, not executing root tests";
      return;
    }
    root_plugin = true;
    
    s.add("min", Setting::TypeGroup);
    s["min"].add("type", Setting::TypeString);
    s["min"]["type"] = "root_minuit";
    
    ParId p0 = vm->createParId("p0");
    ParId p1 = vm->createParId("p1");
    ParIds pars;
    pars.insert(p0);
    pars.insert(p1);
    
    Configuration ctx(vm, SettingWrapper(s["min"], s, rec));
    BOOST_REQUIRE(true);//create checkpoint
    std::auto_ptr<Minimizer> min = PluginManager<Minimizer>::build(ctx);
    BOOST_REQUIRE(min.get());
    ImpossibleFunction f(pars);
    bool exception;
    ParValues start, step;
    std::map<ParId, pair<double, double> > ranges;
    start.set(p0, 0.0).set(p1, 0.0);
    step.set(p0, 1.0).set(p1, 1.0);
    ranges[p0] = make_pair(-100.0, 100.0);
    ranges[p1] = make_pair(-100.0, 100.0);
    try{
        BOOST_REQUIRE(true);//create checkpoint
        min->minimize(f, start, step, ranges);
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

