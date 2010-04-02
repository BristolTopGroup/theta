#include "interface/phys.hpp"
#include "interface/histogram.hpp"
#include "interface/random.hpp"
#include "interface/plugin.hpp"
#include "interface/histogram-function.hpp"

#include "test/utils.hpp"

#include "libconfig/libconfig.h++"

#include <boost/test/unit_test.hpp>

using namespace theta;
using namespace theta::plugin;
using namespace std;


BOOST_AUTO_TEST_SUITE(model_tests)

BOOST_AUTO_TEST_CASE(model0){
    BOOST_CHECKPOINT("model0 entry");
    load_core_plugins();
    
    boost::shared_ptr<VarIdManager> vm(new VarIdManager);
    ParIds pars;
    ObsIds obs;
    const size_t nbins = 100;
    ParId beta1 = vm->createParId("beta1");
    ParId beta2 = vm->createParId("beta2");
    ObsId obs0 = vm->createObsId("obs0", nbins, -1, 1);
    pars.insert(beta1);
    pars.insert(beta2);
    obs.insert(obs0);
    
    //boost::ptr_vector<Function> coeffs;
    BOOST_CHECKPOINT("parsing config");
    
    ConfigCreator cc("flat-histo = {type = \"fixed_poly\"; observable=\"obs0\"; coefficients = [1.0]; normalize_to = 1.0;};\n"
            "gauss-histo = {type = \"fixed_gauss\"; observable=\"obs0\"; width = 0.5; mean = 0.5; normalize_to = 1.0;};\n"
            "c1 = {type = \"mult\"; parameters=(\"beta1\");};\n"
            "c2 = {type = \"mult\"; parameters=(\"beta2\");};\n"
            "dist-flat = {\n"
            "       type = \"flat_distribution\";\n"
            "       beta1 = { range = (\"-inf\", \"inf\"); }; \n"
            "       beta2 = { range = (\"-inf\", \"inf\"); };\n"
            " };\n"
            "m = {\n"
            "  obs0 = {\n"
            "       signal = {\n"
            "          coefficient-function = \"@c1\";\n"
            "          histogram = \"@gauss-histo\";\n"
            "       };\n"
            "       background = {\n"
            "           coefficient-function = \"@c2\";\n"
            "           histogram = \"@flat-histo\";\n"
            "       };\n"
            "   };\n"
            "  parameter-distribution = \"@dist-flat\";\n"
            "};\n"
            , vm);
    
    BOOST_CHECKPOINT("config parsed");
    
    const theta::plugin::Configuration & cfg = cc.get();
    BOOST_CHECKPOINT("building model");
    std::auto_ptr<Model> m;
    try{
        m = ModelFactory::buildModel(Configuration(cfg, cfg.setting["m"]));
    }
    catch(Exception & ex){
        cerr << ex.message << endl;
    }
    catch(libconfig::SettingNotFoundException & ex){
        cerr << ex.getPath() << " not found" << endl;
    }
    
    BOOST_REQUIRE(m.get()!=0);
    
    BOOST_CHECKPOINT("building signal histo");
    std::auto_ptr<HistogramFunction> f_signal_histo = PluginManager<HistogramFunction>::build(Configuration(cfg, cfg.setting["gauss-histo"]));
    BOOST_CHECKPOINT("building bkg histo");
    std::auto_ptr<HistogramFunction> f_bkg_histo = PluginManager<HistogramFunction>::build(Configuration(cfg, cfg.setting["flat-histo"]));
    
    ParValues values;
    Histogram signal = (*f_signal_histo)(values);
    Histogram background = (*f_bkg_histo)(values);

    
    /*BOOST_CHECKPOINT("before coeff building");
    
    coeffs.push_back(PluginManager<Function>::build(Configuration(cfg, setting["c1"])));
    coeffs.push_back(PluginManager<Function>::build(Configuration(cfg, setting["c2"])));
    
    BOOST_CHECKPOINT("before histo building");
    boost::ptr_vector<HistogramFunction> histos;
    Histogram signal(nbins, -1, 1);
    Histogram background(nbins, -1, 1);
    for(size_t i = 1; i<=nbins; i++){
        background.set(i, 0.5);
        double x = 2.0 / 100 * i - 1;
        signal.set(i, exp(-0.5*x*x));
    }
    signal*=10;
    background*=10;
    histos.push_back(new ConstantHistogramFunction(signal));
    histos.push_back(new ConstantHistogramFunction(background));
    
    vector<string> names;
    names.push_back("signal");
    names.push_back("background");*/
    
    BOOST_CHECKPOINT("before setting prediction");
    //m.set_prediction(obs0, coeffs, histos, names);
    values.set(beta1, 1.0);
    values.set(beta2, 0.0);
    Histogram s;
    m->get_prediction(s, values, obs0);
    //s should be signal only:
    for(size_t i = 1; i<=nbins; i++){
        BOOST_REQUIRE(signal.get(i)==s.get(i));
    }
    //background only:
    values.set(beta1, 0.0);
    values.set(beta2, 1.0);
    m->get_prediction(s, values, obs0);
    for(size_t i = 1; i<=nbins; i++){
        BOOST_REQUIRE(background.get(i)==s.get(i));
    }
    //zero prediction:
    values.set(beta1, 0.0);
    values.set(beta2, 0.0);
    m->get_prediction(s, values, obs0);
    for(size_t i = 1; i<=nbins; i++){
        BOOST_REQUIRE(0.0==s.get(i));
    }

    //The likelihood, take double background. Use average as data:
    values.set(beta1, 1.0);
    values.set(beta2, 2.0);
    m->get_prediction(s, values, obs0);
    Data data;
    BOOST_CHECKPOINT("check");
    data[obs0] = s;
    BOOST_CHECKPOINT("check2");
    NLLikelihood nll = m->getNLLikelihood(data);
    BOOST_CHECKPOINT("check3");
    double x[2];
    x[0] = 0.9;
    x[1] = 1.9;
    double nll09 = nll(x);
    x[0] = 1.0;
    x[1] = 2.0;
    double nll10 = nll(x);
    x[0] = 1.1;
    x[1] = 2.1;
    double nll11 = nll(x);
    BOOST_CHECKPOINT("check4");
    BOOST_CHECK(nll10 < nll11);
    BOOST_CHECK(nll10 < nll09);
    //scan the likelihood function:
    /*x[1] = 2.0;
    for(double x0=0.1; x0<3.0; x0+=0.1){
        x[0] = x0;
        printf("nll(%f,%f) = %f\n", x[0], x[1], nll(x));
    }*/
}

/*BOOST_AUTO_TEST_CASE(modelgrad){
    BOOST_REQUIRE(true);
    boost::shared_ptr<VarIdManager> vm(new VarIdManager);
    ParIds pars;
    ObsIds obs;
    const size_t nbins = 100;
    ParId beta1 = vm->createParId("beta1");
    ParId beta2 = vm->createParId("beta2");
    ObsId obs0 = vm->createObsId("obs0", nbins, -1, 1);
    pars.insert(beta1);
    pars.insert(beta2);
    obs.insert(obs0);
    Model m(vm);
    boost::ptr_vector<Function> coeffs;
    
    ConfigCreator cc("b1 = {type = \"mult\"; parameters=(\"beta1\");}; b2 = {type = \"mult\"; parameters=(\"beta2\");};", vm);
    
    const Configuration & config = cc.get();
    const SettingWrapper & setting = config.setting;
    
    coeffs.push_back(PluginManager<Function>::build(Configuration(config, setting["b1"])));
    coeffs.push_back(PluginManager<Function>::build(Configuration(config, setting["b2"])));
    
    boost::ptr_vector<HistogramFunction> histos;

    Histogram signal(nbins, -1, 1);
    Histogram background(nbins, -1, 1);
    BOOST_REQUIRE(true);

    Random rnd(new RandomSourceTaus());
    for(size_t i = 1; i<=nbins; i++){
        //background.set(i, 1.0 + rnd.get());
        //signal.set(i, 1.0 + rnd.get());
        signal.set(i, 1.0);
        background.set(i, 1.0);
    }

    histos.push_back(new ConstantHistogramFunction(signal));
    histos.push_back(new ConstantHistogramFunction(background));
    vector<string> names;
    names.push_back("signal");
    names.push_back("background");
    m.set_prediction(obs0, coeffs, histos, names);

    ParValues values;
    values.set(beta1, 1.0);
    values.set(beta2, 2.0);
    BOOST_REQUIRE(true);

    /*Histogram s;
    m.get_prediction_derivative(s, values, obs0, beta1);
    BOOST_REQUIRE(s.get_nbins() == signal.get_nbins());
    //the derivative w.r.t. beta1 should yield the signal Histogram:
    Histogram s2 = signal;
    for(size_t i=1; i<=s.get_nbins(); i++){
        //printf("%f %f\n", s.get(i), s2.get(i));
        BOOST_REQUIRE(utils::close_to(s.get(i), s2.get(i), 1.0));
    }

    values.set(beta1, 0.1);
    m.get_prediction_derivative(s, values, obs0, beta1);
    s2 = signal;
    for(size_t i=1; i<=s.get_nbins(); i++){
        //printf("%f %f\n", s.get(i), s2.get(i));
        BOOST_REQUIRE(utils::close_to(s.get(i), s2.get(i), 1.0));
    }

    //test NLL derivative:
    m.get_prediction(s, values, obs0);
    BOOST_REQUIRE(true);
    Data data;
    data.addData(obs0, s);
    BOOST_REQUIRE(true);

    NLLikelihood nll = m.getNLLikelihood(data);
    double nll_value;
    double nll_value2;
    boost::scoped_array<double> g(new double[nll.getnpar()]);
    boost::scoped_array<double> g2(new double[nll.getnpar()]);
    double x[2];
    x[0] = 1.0;
    x[1] = 1.2;
    nll.gradient(x, g.get(), nll_value);
    nll.gradient2(x, g2.get(), nll_value2);
    for(size_t i=0; i<nll.getnpar(); i++){
        double rel_error = fabs(g[i] - g2[i]) / max(fabs(g[i]), fabs(g2[i]));
        BOOST_CHECK(rel_error < 0.0001);
    }* /
}*/


BOOST_AUTO_TEST_SUITE_END()
