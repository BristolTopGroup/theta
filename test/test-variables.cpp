#include "interface/random.hpp"
#include "interface/variables.hpp"

#include <boost/test/unit_test.hpp>

using namespace theta;

BOOST_AUTO_TEST_SUITE(variables_tests)


BOOST_AUTO_TEST_CASE(parids){
    VarIdManager vm;
    ParId var0 = vm.createParId("var0");
    BOOST_REQUIRE(vm.parNameExists("var0"));
    BOOST_REQUIRE(vm.getName(var0)=="var0");
    BOOST_REQUIRE(vm.getParId("var0")==var0);
    bool ex = false;
    try{
        vm.getParId("var1");
    }
    catch(NotFoundException &){
        ex = true;
    }
    BOOST_REQUIRE(ex);
    ex = false;
    try{
        vm.createParId("var0", 1.0);
    }
    catch(InvalidArgumentException &){
        ex = true;
    }
    BOOST_REQUIRE(ex);
    ParId var1 = vm.createParId("var1");
    BOOST_REQUIRE(var0!=var1);
    //same id is returned in case of same specification:
    ParId var0_2 = vm.createParId("var0");
    BOOST_REQUIRE(var0==var0_2);
}


BOOST_AUTO_TEST_CASE(parvalues){
    ParValues vv;
    VarIdManager vm;
    ParId v0 = vm.createParId("v0");
    BOOST_REQUIRE(not vv.contains(v0));
    vv.set(v0, -7.0);
    BOOST_REQUIRE(vv.get(v0)==-7.0);
    BOOST_REQUIRE(vv.contains(v0));
    ParId v1 = vm.createParId("v1");
    bool ex = false;
    try{
        vv.get(v1);
    }
    catch(NotFoundException &){
        ex = true;
    }
    BOOST_REQUIRE(ex);
}

BOOST_AUTO_TEST_SUITE_END()
