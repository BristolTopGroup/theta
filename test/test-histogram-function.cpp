#include "interface/histogram-function.hpp"
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace theta;
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
    //now, make an interpolation between different histograms
    // with different gaussian widths:
    Histogram hnarrow(nbins, -1, 1);
    Histogram hwide(nbins, -1, 1);
    for(size_t i = 1; i<=nbins; i++){
        double x = 2.0 / 100 * i - 1;
        hwide.set(i, exp(-0.1*x*x));
        hnarrow.set(i, exp(-2.0*x*x));
    }
    ParId delta = vm.createParId("delta");
    vector<Histogram> hplus, hminus;
    vector<ParId> vdelta;
    hplus.push_back(hnarrow);
    hminus.push_back(hwide);
    vdelta.push_back(delta);
    //todo test interpolating!!
    /*std::auto_ptr<HistogramFunction> hp_sys(new InterpolatingHistogramFunction(h, vdelta, hplus, hminus));

    ParValues values;
    values.set(delta, 0.0);
    //we should get back the nominal template now:
    Histogram htest = (*hp_sys)(values);
    for(size_t i = 1; i<=nbins; i++){
        //printf("%f %f %f     %f\n", hminus[0].get(i), h.get(i), hplus[0].get(i), htest.get(i));
        BOOST_REQUIRE(utils::close_to(htest.get(i), h.get(i), 1.0));
    }

    values.set(delta, 1.0);
    htest = (*hp_sys)(values);
    for(size_t i = 1; i<=nbins; i++){
        BOOST_REQUIRE(utils::close_to(htest.get(i), hplus[0].get(i), 1.0));
    }

    values.set(delta, -1.0);
    htest = (*hp_sys)(values);
    for(size_t i = 1; i<=nbins; i++){
        BOOST_REQUIRE(utils::close_to(htest.get(i), hminus[0].get(i), 1.0));
    }

    values.set(delta, -0.5);
    htest = (*hp_sys)(values);
    for(size_t i = 1; i<=nbins; i++){
        //htest should be between hminus and h (except at x=0, were both are 1.0):
        BOOST_CHECK((h.get(i) - htest.get(i)) * (hminus[0].get(i) - htest.get(i)) <= 0);
    }*/
}

BOOST_AUTO_TEST_SUITE_END()
