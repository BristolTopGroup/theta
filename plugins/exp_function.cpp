#include "plugins/exp_function.hpp"
#include "interface/plugin.hpp"

using namespace std;

exp_function::exp_function(const theta::plugin::Configuration & cfg): pid(cfg.vm->getParId(cfg.setting["parameter"])){
    par_ids.insert(pid);
    lambda = cfg.setting["lambda"];
}

double exp_function::operator()(const theta::ParValues & values) const{
    double val = values.get(pid);
    return exp(lambda * val);
}

REGISTER_PLUGIN(exp_function)
