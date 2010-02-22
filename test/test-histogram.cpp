#include "interface/histogram.hpp"
#include "interface/histogram-function.hpp"
#include "interface/phys.hpp"
#include "interface/utils.hpp"
#include "interface/random.hpp"


#include <boost/test/unit_test.hpp>
#include <iostream>


using namespace std;
using namespace theta;

void check_histos_equal(const Histogram & h1, const Histogram & h2){
    BOOST_REQUIRE(h1.get_nbins()==h2.get_nbins());
    BOOST_REQUIRE(h1.get_xmin()==h2.get_xmin());
    BOOST_REQUIRE(h1.get_xmax()==h2.get_xmax());
    //for the total weight, do not assume too much ...
    BOOST_REQUIRE(utils::close_to_relative(h1.get_sum_of_weights(),h2.get_sum_of_weights()));
    const size_t n = h1.get_nbins();
    for(size_t i=0; i<=n+1; i++){
        BOOST_CHECK(h1.get(i)==h2.get(i));
    }
}

BOOST_AUTO_TEST_SUITE(histogram_tests)

//test construcors and copy assignment
BOOST_AUTO_TEST_CASE(ctest){
   //default construction:
   Histogram m(100, -1, 1);
   BOOST_CHECK(m.get_nbins()==100);
   BOOST_CHECK(m.get_xmin()==-1);
   BOOST_CHECK(m.get_xmax()==1);
   BOOST_CHECK(m.get_sum_of_weights()==0);
   //fill a bit:
   for(size_t i=0; i<=101; i++){
       m.set(i, i*i);
   }

   //copy constructos:
   Histogram mcopy(m);
   BOOST_REQUIRE(mcopy.getData()!=m.getData());
   check_histos_equal(m, mcopy);

   //copy assignment:
   Histogram m200(200, -1, 1);
   BOOST_CHECK(m200.get_nbins()==200);
   m200 = m;
   BOOST_CHECK(m200.get_nbins()==100);
   check_histos_equal(m200, m);

   //copy assignment with empty histo:
   Histogram h_empty;
   h_empty = m;
   check_histos_equal(h_empty, m);
}


//test get, set, fill:
BOOST_AUTO_TEST_CASE(getset){
    const size_t nbins=100;
    Histogram m(nbins, -1, 1);
    volatile double sum = 0.0;
    for(size_t i=0; i<=nbins; i++){
        volatile double a = sqrt(i+0.0);
        sum += a;
        m.set(i, a);
    }

    BOOST_CHECK(m.get_sum_of_weights()==sum);

    for(size_t i=0; i<=nbins; i++){
        volatile double a = sqrt(i+0.0);
        BOOST_CHECK(m.get(i) == a);
    }

    //fill:
    volatile double content = m.get(1);
    m.fill(-0.999, 1.7);
    content += 1.7;
    BOOST_CHECK(content==m.get(1));
    sum += 1.7;
    BOOST_CHECK(sum==m.get_sum_of_weights());

    //fill in underflow:
    content = m.get(0);
    double delta = 10.032;
    m.fill(-1.001, delta);
    content += delta;
    BOOST_CHECK(content==m.get(0));
    sum += delta;
    BOOST_CHECK(sum==m.get_sum_of_weights());

    //fill in overflow:
    content = m.get(nbins+1);
    delta = 7.032;
    m.fill(1.001, delta);
    content += delta;
    BOOST_CHECK(content==m.get(nbins+1));
    sum += delta;
    BOOST_CHECK(sum==m.get_sum_of_weights());
}

//test +=
BOOST_AUTO_TEST_CASE(test_plus){
    RandomTaus rnd;
    Histogram m0(100, 0, 1);
    Histogram m1(m0);
    Histogram m_expected(m0);
    for(size_t i=0; i<=101; i++){
        volatile double g0 = rnd.get();
        m0.set(i, g0);
        volatile double g1 = rnd.get();
        m1.set(i, g1);
        g0 += g1;
        m_expected.set(i, g0);
    }
    //m0+=m1 should add the histogram m1 to m0, leaving m1 untouched ...
    Histogram m1_before(m1);
    Histogram m0_before(m0);
    m0+=m1;
    //the sum of weights could be slightly off.
    check_histos_equal(m0, m_expected);
    check_histos_equal(m1, m1_before);
    //... and it should commute:
    m1+=m0_before;
    check_histos_equal(m1, m_expected);
    check_histos_equal(m1, m0);
}

//test *=
BOOST_AUTO_TEST_CASE(test_multiply){
    RandomTaus rnd;
    Histogram m0(100, 0, 1);
    Histogram m1(m0);
    Histogram m0m1(m0);
    Histogram m0factor_expected(m0);
    double factor = rnd.get();
    for(size_t i=0; i<=101; i++){
        double g0 = rnd.get();
        m0.set(i, g0);
        m0factor_expected.set(i, g0*factor);
        double g1 = rnd.get();
        m1.set(i, g1);
        m0m1.set(i, g0*g1);
    }
    Histogram m0_before(m0);
    m0*=factor;
    check_histos_equal(m0, m0factor_expected);
    m1*=m0_before;
    check_histos_equal(m1, m0m1);
}

BOOST_AUTO_TEST_SUITE_END()
