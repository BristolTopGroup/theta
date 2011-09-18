#include "interface/histogram.hpp"
#include "interface/phys.hpp"
#include "interface/variables.hpp"

#include <boost/test/unit_test.hpp>

using namespace theta;
using namespace std;

BOOST_AUTO_TEST_SUITE(data_tests)


BOOST_AUTO_TEST_CASE(basic){
    VarIdManager vm;
    ObsId oid1 = vm.createObsId("oid1", 10, 0, 1);
    ObsId oid3 = vm.createObsId("oid3", 10, 0, 1);
    ObsId oid5 = vm.createObsId("oid5", 10, 0, 1);
    
    Histogram1D h(10, 0, 1);

    Data d;
    ObsIds oids = d.getObservables();
    BOOST_CHECK(oids == ObsIds());
    
    oids.insert(oid1);
    d[oid1] = h;
    BOOST_CHECK(oids == d.getObservables());
    d[oid3] = h;
    oids.insert(oid3);
    BOOST_CHECK(oids == d.getObservables());
    
    //try to access non-existent through const:
    const Data & c_d = d;
    bool ex = false;
    try{
       c_d[oid5];
    }
    catch(Exception &){
      ex = true;
    }
    BOOST_CHECK(ex);
}


BOOST_AUTO_TEST_SUITE_END()
