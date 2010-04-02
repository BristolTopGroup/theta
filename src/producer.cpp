#include "interface/producer.hpp"
#include "interface/phys.hpp"
#include "interface/histogram.hpp"
#include "interface/distribution.hpp"

using namespace theta;
using namespace theta::plugin;

Producer::Producer(const Configuration & cfg): PluginType(cfg){
    if(cfg.setting.exists("add-nllikelihood-functions")){
        SettingWrapper s = cfg.setting["add-nllikelihood-functions"];
        size_t n = s.size();
        if(n==0){
            throw ConfigurationException("Producer: empty add-nllikelihood-functions specified");
        }
        for(size_t i=0; i<n; ++i){
            additional_likelihood_functions.push_back(PluginManager<Function>::build(Configuration(cfg, s[i])));
        }
    }
}

NLLikelihood Producer::get_nllikelihood(const Data & data, const Model & model){
    NLLikelihood nll = model.getNLLikelihood(data);
    if(additional_likelihood_functions.size()){
        nll.set_additional_terms(&additional_likelihood_functions);
    }
    return nll;
}

/*ProducerLikelihoodPrinciple::ProducerLikelihoodPrinciple(const plugin::Configuration & cfg): Producer(cfg){
}

void ProducerLikelihoodPrinciple::produce(Run & run, const Data & data, const Model & model){
    if(ref_model.get()==0)ref_model.reset(new t_ref_model(model));
    produce(run, get_nllikelihood(data, model));
}

NLLikelihood ProducerLikelihoodPrinciple::get_asimov_likelihood(){
    const Model& model = (*ref_model).model;
    //as the NLLikelihood object holds references to Data and Model, we must use
    // data and model instances which are not local to this function ...
    asimov_data.reset(new Data());
    ObsIds observables = model.getObservables();
    ParValues mode_values;
    model.get_parameter_distribution().mode(mode_values);
    for(ObsIds::const_iterator it=observables.begin(); it!=observables.end(); ++it){
        Histogram h;
        model.get_prediction((*asimov_data)[*it], mode_values, *it);
    }
    return model.getNLLikelihood(*asimov_data);
}*/

