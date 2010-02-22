#include "interface/random.hpp"

#include <boost/test/unit_test.hpp>

using namespace theta;

BOOST_AUTO_TEST_SUITE(rndtaus_tests)

//test the copied randomtaus versus the expected results directly from GSL taus2.
BOOST_AUTO_TEST_CASE(randomtaus){
    RandomTaus rnd;
    unsigned int first_expected [10] = {802792108u, 4084684829u, 2342628799u, 320516809u, 984487517u, 2246144618u,
       398433606u, 2198246467u, 1456873311u, 3409412117u};
    for(int i=0; i<10; i++){
       BOOST_REQUIRE(rnd.get() == first_expected[i]);
    }
    rnd.setSeed(123456789u);
    BOOST_REQUIRE(rnd.get() == 3426689362u);
    for(int i=0; i<10; i++){
       rnd.setSeed(rnd.get());
    }
    BOOST_REQUIRE(rnd.get() == 200083673u);
}

BOOST_AUTO_TEST_SUITE_END()
