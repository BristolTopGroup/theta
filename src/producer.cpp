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
    if(cfg.setting.exists("nllikelihood-add-functions")){
        SettingWrapper s = cfg.setting["nllikelihood-add-functions"];
        size_t n = s.size();
        if(n==0){
            throw ConfigurationException("Producer: empty nllikelihood-add-functions specified");
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

ProducerLikelihoodPrinciple::ProducerLikelihoodPrinciple(const plugin::Configuration & cfg): Producer(cfg){
}

void ProducerLikelihoodPrinciple::produce(Run & run, const Data & data, const Model & model){
    ref_datamodel.reset(new t_ref_datamodel(data, model));
    produce(run, get_nllikelihood(data, model));
}

NLLikelihood ProducerLikelihoodPrinciple::get_asimov_likelihood(){
    const Model& model = (*ref_datamodel).model;
    asimov_data.reset(new Data());
    ObsIds observables = model.getObservables();
    ParValues mode_values;
    model.get_parameter_distribution().mode(mode_values);
    for(ObsIds::const_iterator it=observables.begin(); it!=observables.end(); ++it){
        Histogram h;
        model.get_predition(h, mode_values, *it);
        (*asimov_data).addData(*it, h);
    }
    return model.getNLLikelihood(*asmiov_data);
}

