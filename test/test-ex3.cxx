//test exceptions during Factory::build

#include "interface/phys.hpp"
#include "interface/plugin_so_interface.hpp"
#include "interface/plugin.hpp"

#include <string>

using namespace std;
using namespace theta;
using namespace theta::plugin;

class TestExDuringBuild: public FunctionFactory{
public:
  auto_ptr<Function> build(ConfigurationContext & ctx) const {
       throw Exception("exception message 23");
  }
  
  string getTypeName() const{
     return "test-exception-during-build";
  }
};

REGISTER_FACTORY(TestExDuringBuild)
