#include "interface/producer.hpp"

using namespace theta;
using namespace theta::plugin;

Producer::Producer(const Configuration & cfg): PluginType(cfg){
    if(cfg.setting.exists("fix-parameters")){
        fix_parameters = Plugin<ParameterSource>::build(Configuration(cfg, cfg.setting["fix-parameters"]));
    }
    if(cfg.setting.exists("additional-likelihood-terms")){
        SettingWrapper s = cfg.setting["additional-likelihood-terms"];
        size_t n = s.size();
        if(n==0){
            throw ConfigurationException("Producer: empty additional-likelihood-terms specified");
        }
        for(size_t i=0; i<n; ++i){
            additional_likelihood_terms.push_back(Plugin<Function>::build(Configuration(cfg, s[i])));
        }
    }
}
