#include "interface/phys.hpp"

#include <boost/test/unit_test.hpp>

using namespace theta;
using namespace std;


BOOST_AUTO_TEST_SUITE(model_tests)

BOOST_AUTO_TEST_CASE(model0){
    boost::shared_ptr<VarIdManager> vm(new VarIdManager);
    ParIds pars;
    ObsIds obs;
    const size_t nbins = 100;
    ParId beta1 = vm->createParId("beta1", 1.0);
    ParId beta2 = vm->createParId("beta2", 1.0);
    ObsId obs0 = vm->createObsId("obs0", nbins, -1, 1);
    pars.insert(beta1);
    pars.insert(beta2);
    obs.insert(obs0);
    Model m(vm);
    boost::ptr_vector<Function> coeffs;
    ParIds v_beta1;
    v_beta1.insert(beta1);
    ParIds v_beta2;
    v_beta2.insert(beta2);
    coeffs.push_back(new MultFunction(v_beta1));
    coeffs.push_back(new MultFunction(v_beta2));
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
    names.push_back("background");
    //TODO check error behavior, e.g. likelihood without any histos set!
    m.setPrediction(obs0, coeffs, histos, names);
    ParValues values;
    values.set(beta1, 1.0);
    values.set(beta2, 0.0);
    Histogram s;
    m.get_prediction(s, values, obs0);
    //s should be signal only:
    for(size_t i = 1; i<=nbins; i++){
        BOOST_REQUIRE(signal.get(i)==s.get(i));
    }
    //background only:
    values.set(beta1, 0.0);
    values.set(beta2, 1.0);
    m.get_prediction(s, values, obs0);
    for(size_t i = 1; i<=nbins; i++){
        BOOST_REQUIRE(background.get(i)==s.get(i));
    }
    //zero prediction:
    values.set(beta1, 0.0);
    values.set(beta2, 0.0);
    m.get_prediction(s, values, obs0);
    for(size_t i = 1; i<=nbins; i++){
        BOOST_REQUIRE(0.0==s.get(i));
    }

    //The likelihood, take double background. Use average as data:
    values.set(beta1, 1.0);
    values.set(beta2, 2.0);
    m.get_prediction(s, values, obs0);
    Data data;
    data.addData(obs0, s);
    NLLikelihood nll = m.getNLLikelihood(data);
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
    BOOST_CHECK(nll10 < nll11);
    BOOST_CHECK(nll10 < nll09);
    //scan the likelihood function:
    /*x[1] = 2.0;
    for(double x0=0.1; x0<3.0; x0+=0.1){
        x[0] = x0;
        printf("nll(%f,%f) = %f\n", x[0], x[1], nll(x));
    }*/
}

BOOST_AUTO_TEST_CASE(modelgrad){
    BOOST_REQUIRE(true);
    boost::shared_ptr<VarIdManager> vm(new VarIdManager);
    ParIds pars;
    ObsIds obs;
    const size_t nbins = 100;
    ParId beta1 = vm->createParId("beta1", 1.0);
    ParId beta2 = vm->createParId("beta2", 1.0);
    ObsId obs0 = vm->createObsId("obs0", nbins, -1, 1);
    pars.insert(beta1);
    pars.insert(beta2);
    obs.insert(obs0);
    Model m(vm);
    boost::ptr_vector<Function> coeffs;
    ParIds v_beta1;
    v_beta1.insert(beta1);
    ParIds v_beta2;
    v_beta2.insert(beta2);
    coeffs.push_back(new MultFunction(v_beta1));
    coeffs.push_back(new MultFunction(v_beta2));
    boost::ptr_vector<HistogramFunction> histos;

    Histogram signal(nbins, -1, 1);
    Histogram background(nbins, -1, 1);
    BOOST_REQUIRE(true);

    RandomTaus rnd;
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
    m.setPrediction(obs0, coeffs, histos, names);

    ParValues values;
    values.set(beta1, 1.0);
    values.set(beta2, 2.0);
    BOOST_REQUIRE(true);

    Histogram s;
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
    }
}


BOOST_AUTO_TEST_SUITE_END()
