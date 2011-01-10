#include "interface/producer.hpp"
#include "interface/model.hpp"
#include "interface/phys.hpp"

using namespace theta;
using namespace theta::plugin;

REGISTER_PLUGIN_BASETYPE(Producer);

Producer::Producer(const Configuration & cfg): ProductsTableWriter(cfg){
    if(cfg.setting.exists("override-parameter-distribution")){
        override_parameter_distribution = PluginManager<Distribution>::instance().build(Configuration(cfg, cfg.setting["override-parameter-distribution"]));
    }
    if(cfg.setting.exists("additional-nll-term")){
        additional_nll_term = PluginManager<Function>::instance().build(Configuration(cfg, cfg.setting["additional-nll-term"]));
    }
}

std::auto_ptr<NLLikelihood> Producer::get_nllikelihood(const Data & data, const Model & model){
    std::auto_ptr<NLLikelihood> nll = model.getNLLikelihood(data);
    nll->set_override_distribution(override_parameter_distribution);
    nll->set_additional_term(additional_nll_term);
    return nll;
}
