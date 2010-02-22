//this is a minimal FunctionPlugin used by test-exception.hpp
// to test inter-compilatio unit exception catching with shared objects.

#include "interface/phys.hpp"
#include "interface/plugin_so_interface.hpp"
#include "interface/plugin.hpp"

#include <string>

using namespace std;
using namespace theta;
using namespace theta::plugin;

class TestExFunction: public Function{
public:
    TestExFunction(): Function(ParIds()){}
    virtual double operator()(const ParValues & v) const{
       throw Exception("test-exception message 42");
    }
    virtual double gradient(const ParValues & v, const ParId & pid) const{ return 0.0;}
};

class TestExFunctionFactory: public FunctionFactory{
public:
  auto_ptr<Function> build(ConfigurationContext & ctx) const {
     return auto_ptr<Function>(new TestExFunction());
  }
  
  string getTypeName() const{
     return "test-exception";
  }

};

REGISTER_FACTORY(TestExFunctionFactory)
