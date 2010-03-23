#include "plugins/one_over_sqrt.hpp"
#include "interface/exception.hpp"
#include "interface/plugin.hpp"

using namespace std;

one_over_sqrt::one_over_sqrt(const theta::plugin::Configuration & cfg){
    string parameter_name = cfg.setting["parameter"];
    pid = cfg.vm->getParId(parameter_name);
    par_ids.insert(pid);
}

double one_over_sqrt::operator()(const theta::ParValues & values) const{
    double val = values.get(pid);
    if(val <= 0.0) throw theta::MathException("one_over_sqrt: negative argument");
    return 1.0 / sqrt(val);
}

REGISTER_PLUGIN(one_over_sqrt)
