#include "interface/producer.hpp"
#include "interface/phys.hpp"
//#include "interface/variables.hpp"
#include "interface/histogram.hpp"

using namespace theta;
using namespace theta::plugin;

Producer::Producer(const Configuration & cfg): PluginType(cfg){
    if(cfg.setting.exists("fix-parameters")){
        fix_parameters = PluginManager<ParameterSource>::build(Configuration(cfg, cfg.setting["fix-parameters"]));
    }
    if(cfg.setting.exists("add-nllikelihood-functions")){
        SettingWrapper s = cfg.setting["add-nllikelihood-functions"];
        size_t n = s.size();
        if(n==0){
            throw ConfigurationException("Producer: empty additional-likelihood-terms specified");
        }
        for(size_t i=0; i<n; ++i){
            additional_likelihood_terms.push_back(PluginManager<Function>::build(Configuration(cfg, s[i])));
        }
    }
}

NLLikelihood Producer::get_nllikelihood(const Data & data, const Model & model){
    NLLikelihood nll = model.getNLLikelihood(data);
    if(additional_likelihood_terms.size()>0){
        nll.set_additional_terms(&additional_likelihood_terms);
    }
    return nll;
}
