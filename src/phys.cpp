#include "interface/phys.hpp"
#include "interface/exception.hpp"
#include "interface/utils.hpp"
#include "interface/cfg-utils.hpp"
#include "interface/histogram-function.hpp"
#include "interface/distribution.hpp"


#include "libconfig/libconfig.h++"

#include <limits>
#include <sstream>
#include <iostream>

#include <boost/scoped_array.hpp>

using namespace std;
using namespace theta;
using namespace theta::utils;
using namespace theta::plugin;

/* DATA */
ObsIds Data::getObservables() const{
    ObsIds result;
    std::map<ObsId, Histogram>::const_iterator it = data.begin();
    for(;it!=data.end(); it++){
        result.insert(it->first);
    }
    return result;
}

/* MODEL */
Model::Model(const boost::shared_ptr<VarIdManager> & vm_):
                   vm(vm_){
}

ParIds Model::getParameters() const{
    return parameters;
}

ObsIds Model::getObservables() const{
    return observables;
}

void Model::samplePseudoData(Data & result, Random & rnd, const ParValues & values) const{
   for (ObsIds::const_iterator it = observables.begin(); it != observables.end(); it++) {
       Histogram pred;
       get_prediction_randomized(rnd, pred, values, *it);
       pred.fill_with_pseudodata(result[*it], rnd);
   }
}

void Model::set_prediction(const ObsId & obs_id, boost::ptr_vector<Function> & coeffs_, boost::ptr_vector<HistogramFunction> & histos_, const std::vector<std::string> & component_names){
    observables.insert(obs_id);
    const size_t n = coeffs_.size();
    if(n!=coeffs_.size() or n!=component_names.size()) throw InvalidArgumentException("Model::setPrediction: number of histograms, coefficients and component names do not match!");
    if(histos[obs_id].size()>0)
        throw InvalidArgumentException("Model::setPrediction: prediction already set for this observable!");
    histos[obs_id].transfer(histos[obs_id].end(), histos_.begin(), histos_.end(), histos_);
    if(coeffs[obs_id].size()>0)
        throw InvalidArgumentException("Model::setPrediction: prediction already set for this observable (coeffs)!");
    coeffs[obs_id].transfer(coeffs[obs_id].end(), coeffs_.begin(), coeffs_.end(), coeffs_);
    names[obs_id] = component_names;
    for(boost::ptr_vector<Function>::const_iterator it=coeffs[obs_id].begin(); it!=coeffs[obs_id].end(); ++it){
        ParIds pids = (*it).getParameters();
        parameters.insert(pids.begin(), pids.end());
    }
    for(boost::ptr_vector<HistogramFunction>::const_iterator it=histos[obs_id].begin(); it!=histos[obs_id].end(); ++it){
        ParIds pids = (*it).getParameters();
        parameters.insert(pids.begin(), pids.end());
    }
}

void Model::get_prediction(Histogram & result, const ParValues & parameters, const ObsId & obs_id) const {
    histos_type::const_iterator it = histos.find(obs_id);
    if (it == histos.end()) throw InvalidArgumentException("Model::getPrediction: invalid obs_id");
    //const boost::ptr_vector<HistogramFunction> & h_producers = it->second;
    histos_type::const_mapped_reference h_producers = *(it->second);
    coeffs_type::const_iterator it2 = coeffs.find(obs_id);
    coeffs_type::const_mapped_reference h_coeffs = *(it2->second);
    bool result_init = false;
    for (size_t i = 0; i < h_producers.size(); i++) {
        if(result_init){
            result.add_with_coeff(h_coeffs[i](parameters), h_producers[i](parameters));
        }
        else{
            result = h_producers[i](parameters);
            result *= h_coeffs[i](parameters);
            result_init = true;
        }
    }
    if(not result_init){
        const pair<double, double> & o_range = vm->get_range(obs_id);
        result.reset(vm->get_nbins(obs_id), o_range.first, o_range.second);
    }
}

void Model::get_prediction_randomized(Random & rnd, Histogram &result, const ParValues & parameters, const ObsId & obs_id) const{
    histos_type::const_iterator it = histos.find(obs_id);
    if (it == histos.end()) throw InvalidArgumentException("Model::get_prediction_randomized: invalid obs_id");
    histos_type::const_mapped_reference h_producers = *(it->second);
    coeffs_type::const_iterator it2 = coeffs.find(obs_id);
    coeffs_type::const_mapped_reference h_coeffs = *(it2->second);
    bool result_init = false;
    for (size_t i = 0; i < h_producers.size(); i++) {
        if(result_init){
            result.add_with_coeff(h_coeffs[i](parameters), h_producers[i].getRandomFluctuation(rnd, parameters));
        }
        else{
            result = h_producers[i].getRandomFluctuation(rnd, parameters);
            result *= h_coeffs[i](parameters);
            result_init = true;
        }
    }
    if(not result_init){
        const pair<double, double> & o_range = vm->get_range(obs_id);
        result.reset(vm->get_nbins(obs_id), o_range.first, o_range.second);
    }
}

size_t Model::getPredictionNComponents(const ObsId & obs_id) const{
    histos_type::const_iterator it = histos.find(obs_id);
    if(it==histos.end()) throw InvalidArgumentException("Model::getPrediction: invalid obs_id");
    return it->second->size();
}

std::string Model::getPredictionComponentName(const ObsId & obs_id, size_t i) const{
    std::map<ObsId, vector<string> >::const_iterator it = names.find(obs_id);
    if(it==names.end()) throw InvalidArgumentException("Model::getPredictionComponentName: invalid obs_id");
    const vector<string> & h_names = it->second;
    if(i>=h_names.size()) throw NotFoundException("Model::getPredictionComponentName: i out of bounds");
    return h_names[i];
}

Histogram Model::getPredictionComponent(const ParValues & parameters, const ObsId & obs_id, size_t i) const{
    histos_type::const_iterator it = histos.find(obs_id);
    if(it==histos.end()) throw InvalidArgumentException("Model::getPrediction: invalid obs_id");
    histos_type::const_mapped_reference h_producers = *(it->second);
    if(i>=h_producers.size()) throw NotFoundException("Model::getPredictionComponent: parameter i out of bounds");
    coeffs_type::const_iterator it2 = coeffs.find(obs_id);
    coeffs_type::const_mapped_reference h_coeffs = *(it2->second);
     
    Histogram result = h_producers[i](parameters);
    result *= h_coeffs[i](parameters);
    return result;
}

NLLikelihood Model::getNLLikelihood(const Data & data) const{
    if(not(data.getObservables()==observables)){
        throw InvalidArgumentException("Model::createNLLikelihood: observables of model and data mismatch!");
    }
    return NLLikelihood(*this, data, observables);
}

/* ModelFactory */
std::auto_ptr<Model> ModelFactory::buildModel(const Configuration & ctx) {
    SettingWrapper s = ctx.setting;
    std::auto_ptr<Model> result(new Model(ctx.vm));
    ObsIds observables = ctx.vm->getAllObsIds();
    //go through observables to find the template definition for each of them:
    for (ObsIds::const_iterator obsit = observables.begin(); obsit != observables.end(); obsit++) {
        string obs_name = ctx.vm->getName(*obsit);
        if(not s.exists(obs_name)) continue;
        SettingWrapper obs_setting = s[obs_name];
        boost::ptr_vector<HistogramFunction> histos;
        boost::ptr_vector<Function> coeffs;
        vector<string> names;
        for (size_t i = 0; i < obs_setting.size(); i++) {
            Configuration context(ctx, obs_setting[i]["histogram"]);
            auto_ptr<HistogramFunction> hf = PluginManager<HistogramFunction>::build(context);
            Configuration cfg(ctx, obs_setting[i]["coefficient-function"]);
            auto_ptr<Function> coeff_function = PluginManager<Function>::build(cfg);
            coeffs.push_back(coeff_function);
            assert(coeff_function.get()==0);
            histos.push_back(hf);
            names.push_back(obs_setting[i].getName());
        }
        result->set_prediction(*obsit, coeffs, histos, names);
    }
    result->parameter_distribution = PluginManager<Distribution>::build(Configuration(ctx, s["parameter-distribution"]));
    ParIds d_pars = result->parameter_distribution->getParameters();
    ParIds m_pars = result->getParameters();
    //check that d_pars is a subset of m_pars
    if(not d_pars.contains_all(m_pars)){
        throw ConfigurationException("parameter-distribution does not define all model parameters!");
    }
    return result;
}


/* NLLIKELIHOOD */
NLLikelihood::NLLikelihood(const Model & m, const Data & dat, const ObsIds & obs): model(m),
        data(dat), obs_ids(obs), additional_terms(0), override_distribution(0){
    Function::par_ids = model.parameters;
    for(ParIds::const_iterator it=par_ids.begin(); it!=par_ids.end(); ++it){
        ranges[*it] = model.parameter_distribution->support(*it);
    }
}

void NLLikelihood::set_additional_terms(const boost::ptr_vector<Function> * terms){
    additional_terms = terms;
}

void NLLikelihood::set_override_distribution(const Distribution * d){
    override_distribution = d;
    if(d){
        for(ParIds::const_iterator it=par_ids.begin(); it!=par_ids.end(); ++it){
            ranges[*it] = d->support(*it);
        }
    }else{
        for(ParIds::const_iterator it=par_ids.begin(); it!=par_ids.end(); ++it){
            ranges[*it] = model.parameter_distribution->support(*it);
        }
    }
}


double NLLikelihood::operator()(const ParValues & values) const{
    //check ranges and return infinity if violated.
    //
    //Unfortunately, this is not as straight-forward as it sounds
    // as some minimizers need numerical derivatives at the end of ranges and do
    // so without respecting these ranges (MINUIT). Therefore, allow some 
    // border-violating evaluations.
    for(ParIds::const_iterator p_it = par_ids.begin(); p_it != par_ids.end(); ++p_it){
        const map<ParId, pair<double, double> >::const_iterator range_it = ranges.find(*p_it);
        if(range_it==ranges.end())continue;
        double value = values.get(*p_it);
        if(value < (1 - 1e-4) * range_it->second.first || value > (1 + 1e-4) * range_it->second.second)
            return numeric_limits<double>::infinity();
    }
    double result = 0.0;
    //go through all observables:
    for(ObsIds::const_iterator obsit=obs_ids.begin(); obsit!=obs_ids.end(); obsit++){
        const ObsId & obs_id = *obsit;
        Histogram model_prediction;
        try{
            model.get_prediction(model_prediction, values, obs_id);
        }catch(Exception & ex){
            ex.message += " (in NLLikelihood::operator() model.get_prediction())";
            throw;
        }
        const Histogram & data_hist = data[obs_id];
        const size_t nbins = model_prediction.get_nbins();
        const double * __restrict pred_data = model_prediction.getData();
        const double * __restrict data_data = data_hist.getData();
        for(size_t i=1; i<=nbins; i++){
            //if both, the prediction and the data are zero, that does not contribute.
            // However, if the prediction is zero and we have non-zero data, we MUST return infinity (!) ...
             if(pred_data[i]>0.0)
                 result -= data_data[i] * theta::utils::log(pred_data[i]);
             else if(data_data[i] > 0.0)
                 return numeric_limits<double>::infinity();
        }
        result += model_prediction.get_sum_of_bincontents();
    }
    assert(not std::isnan(result));
    //the model prior:
    if(override_distribution){
        result += override_distribution->evalNL(values);
    }
    else{
        result += model.parameter_distribution->evalNL(values);
    }
    assert(not std::isnan(result));
    //the additional likelihood terms, if set:
    if(additional_terms){
        for(size_t i=0; i < additional_terms->size(); i++){
            result += ((*additional_terms)[i])(values);
        }
    }
    assert(not std::isnan(result));
    return result;
}

