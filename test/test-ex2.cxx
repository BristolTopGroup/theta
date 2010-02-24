//this is a minimal FunctionPlugin. This plugin creates another plugin,
// defined in test-exception.cxx and calls the function there.

#include "interface/phys.hpp"
#include "interface/plugin_so_interface.hpp"
#include "interface/plugin.hpp"

#include <string>

using namespace std;
using namespace theta;
using namespace theta::plugin;

class ProxyFunction: public Function{
public:
    ProxyFunction(auto_ptr<Function> & f_): Function(f_->getParameters()), f(f_){}
    virtual double operator()(const ParValues & v) const{
       try{
           return (*f)(v);
       }
       catch(Exception & ex){
           ex.message = "exception caught by ProxyFunction: " + ex.message;
           throw;
       }
    }
    virtual double gradient(const ParValues & v, const ParId & pid) const{
        return f->gradient(v, pid);
    }
private:
   auto_ptr<Function> f;
};

class TestExFunctionFactory2: public FunctionFactory{
public:
  auto_ptr<Function> build(ConfigurationContext & ctx) const {
     ConfigurationContext ctx2(ctx,ctx.setting["block"]);
     auto_ptr<Function> f = PluginManager<FunctionFactory>::get_instance()->build(ctx2);
     return auto_ptr<Function>(new ProxyFunction(f));
  }
  
  string getTypeName() const{
     return "test-exception2";
  }

};

REGISTER_FACTORY(TestExFunctionFactory2)
