#include "interface/histogram-function.hpp"
#include "interface/plugin.hpp"
#include "interface/histogram.hpp"
#include "interface/variables.hpp"
#include "test/utils.hpp"
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace theta;
using namespace theta::plugin;
using namespace std;

BOOST_AUTO_TEST_SUITE(histogram_function_tests)

BOOST_AUTO_TEST_CASE(histofunction){
    VarIdManager vm;
    size_t nbins = 100;
    Histogram h(nbins, -1, 1);
    for(size_t i = 1; i<=nbins; i++){
        double x = 2.0 / 100 * i - 1;
        h.set(i, exp(-0.5*x*x));
    }
    std::auto_ptr<HistogramFunction> hp(new ConstantHistogramFunction(h));
    Histogram hh = (*hp)(ParValues());
    for(size_t i = 1; i<=nbins; i++){
        BOOST_REQUIRE(h.get(i) == hh.get(i));
    }
}


BOOST_AUTO_TEST_CASE(cubiclinear_histomorph){
    load_core_plugins();
    boost::shared_ptr<VarIdManager> vm(new VarIdManager);
    
    const size_t nbins = 1;
    ParId delta = vm->createParId("delta");
    ObsId obs = vm->createObsId("obs", nbins, -1, 1);
    BOOST_CHECKPOINT("parsing config");
    ConfigCreator cc("flat-histo0 = {type = \"fixed_poly\"; observable=\"obs\"; coefficients = [1.0]; normalize_to = 1.0;};\n"
            "flat-histo1 = {type = \"fixed_poly\"; observable=\"obs\"; coefficients = [1.0]; normalize_to = 1.12;};\n"
            "flat-histo-1 = {type = \"fixed_poly\"; observable=\"obs\"; coefficients = [1.0]; normalize_to = 0.83;};\n"
            "histo = { type = \"cubiclinear_histomorph\"; parameters = (\"delta\"); nominal-histogram = \"@flat-histo0\";\n"
            "      delta-plus-histogram = \"@flat-histo1\"; delta-minus-histogram = \"@flat-histo-1\";\n"
            "};\n"
            , vm);
            
    const theta::plugin::Configuration & cfg = cc.get();
    BOOST_CHECKPOINT("building hf");
    std::auto_ptr<HistogramFunction> hf = PluginManager<HistogramFunction>::instance().build(Configuration(cfg, cfg.setting["histo"]));
    BOOST_CHECKPOINT("hf built");
    ParValues pv;
    //check central and +- 1 sigma values:
    pv.set(delta, 0.0);
    Histogram h = (*hf)(pv);
    BOOST_ASSERT(utils::close_to_relative(h.get(1), 1.0));
    pv.set(delta, 1.0);
    h = (*hf)(pv);
    BOOST_ASSERT(utils::close_to_relative(h.get(1), 1.12));
    pv.set(delta, -1.0);
    h = (*hf)(pv);
    BOOST_ASSERT(utils::close_to_relative(h.get(1), 0.83));
    //+- 2 sigma values, should be interpolated linearly:
    pv.set(delta, 2.0);
    h = (*hf)(pv);
    BOOST_ASSERT(utils::close_to_relative(h.get(1), 1.24));
    pv.set(delta, -2.0);
    h = (*hf)(pv);
    BOOST_ASSERT(utils::close_to_relative(h.get(1), 0.66));
    //cutoff at zero:
    pv.set(delta, -10.0);
    h = (*hf)(pv);
    BOOST_ASSERT(h.get(1) == 0.0);
    //derivative at zero should be smooth:
    pv.set(delta, 1e-8);
    h = (*hf)(pv);
    double eps = h.get(1);
    pv.set(delta, -1e-8);
    h = (*hf)(pv);
    double eps_minus = h.get(1);
    BOOST_ASSERT(utils::close_to(eps - 1, 1 - eps_minus, 1000.0));
    //derivative at zero should be (0.12 + 0.17) / 2.
    double der = (eps - eps_minus) / (2e-8);
    BOOST_ASSERT(fabs(der - (0.12 + 0.17)/2) < 1e-8);
}

BOOST_AUTO_TEST_SUITE_END()
