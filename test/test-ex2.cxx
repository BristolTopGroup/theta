//this is a minimal FunctionPlugin. This plugin creates another plugin,
// defined in test-exception.cxx and calls the function there.

#include "interface/phys.hpp"
#include "interface/plugin_so_interface.hpp"
#include "interface/plugin.hpp"

#include <string>

using namespace std;
using namespace theta;
using namespace theta::plugin;

class proxy_function: public Function{
public:
    proxy_function(const Configuration & cfg): Function(ParIds()){
         Configuration ctx2(cfg,cfg.setting["block"]);
         f = PluginManager<Function>::build(ctx2);
         par_ids = f->getParameters();
    }
    
    virtual double operator()(const ParValues & v) const{
       try{
           return (*f)(v);
       }
       catch(Exception & ex){
           ex.message = "exception caught by proxy_function: " + ex.message;
           throw;
       }
    }
private:
   auto_ptr<Function> f;
};

REGISTER_PLUGIN(proxy_function)
