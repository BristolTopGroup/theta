#include "interface/distribution.hpp"
#include "interface/random.hpp"

#include <boost/test/unit_test.hpp>

using namespace theta;

BOOST_AUTO_TEST_SUITE(distribution_tests)

BOOST_AUTO_TEST_CASE(distribution_lognormal){
    VarIdManager vm;
    ParId var0 = vm.createParId("var0");
    double sigma = .5;
    double mu = 2.0;
    std::auto_ptr<Distribution> d(new LogNormalDistribution(var0, mu, sigma));
    //must return +infinity for argument < 0:
    ParValues values;
    values.set(var0, -1.0);
    double result = d->evalNL(values);
    BOOST_CHECK(isinf(result) and result>0);
    values.set(var0, -10.0);
    result = d->evalNL(values);
    BOOST_CHECK(isinf(result) and result>0);
    values.set(var0, -0.01);
    result = d->evalNL(values);
    BOOST_CHECK(isinf(result) and result>0);
    //calculate independently some values and compare. Comparison
    // is meaningful only for ratios, as the results from Distribution are
    // not required to be normalized.
    //Calculate reference at x=mu:
    double tmp = (utils::log(mu) - mu)/sigma;
    double lognormal_mu = 1.0/mu * exp(-0.5*tmp*tmp); // forget about the factor 1 over sigma sqrt (2 pi) ...
    //same on d:
    values.set(var0, mu);
    double lognormal_mu_d = d->evalNL(values);
    for(double x=0.1; x<10.0; x+=0.1){
        tmp = (utils::log(x) - mu)/sigma;
        double lognormal_x = 1.0/x * exp(-0.5*tmp*tmp);
        //the same on the Distribution object:
        values.set(var0, x);
        double lognormal_x_d = d->evalNL(values);
        BOOST_CHECK(utils::close_to(-utils::log(lognormal_x / lognormal_mu), lognormal_x_d - lognormal_mu_d, 10));
    }
    RandomTaus rnd;
    RandomProxy<RandomTaus> rndproxy(rnd);
    ParValues v_sampled(vm);
    d->sample(v_sampled, rndproxy, vm);
    BOOST_REQUIRE(v_sampled.contains(var0));
    //BOOST_REQUIRE(v_sampled.getAllParIds().size() == 1);
    //it is difficult to check that the sampling works correctly.
    //TODO: do that anyway .... We could check some of the first moments ...
    // Or bin it and make chi^2 for the bins with enough statistics ...
}


BOOST_AUTO_TEST_SUITE_END()
