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
}

BOOST_AUTO_TEST_SUITE_END()
