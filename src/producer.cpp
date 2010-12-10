#include "interface/producer.hpp"
#include "interface/model.hpp"
#include "interface/phys.hpp"

using namespace theta;
using namespace theta::plugin;

REGISTER_PLUGIN_BASETYPE(Producer);

Producer::Producer(const Configuration & cfg): ProductsTableWriter(cfg){
    if(cfg.setting.exists("override-parameter-distribution")){
        override_parameter_distribution = PluginManager<Distribution>::build(Configuration(cfg, cfg.setting["override-parameter-distribution"]));
    }
}

std::auto_ptr<NLLikelihood> Producer::get_nllikelihood(const Data & data, const Model & model){
    std::auto_ptr<NLLikelihood> nll = model.getNLLikelihood(data);
    nll->set_override_distribution(override_parameter_distribution.get());
    return nll;
}
