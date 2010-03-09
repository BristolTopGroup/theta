#include "interface/phys.hpp"
#include "interface/exception.hpp"
#include "interface/utils.hpp"
#include "interface/cfg-utils.hpp"
#include "interface/histogram-function.hpp"
#include "interface/distribution.hpp"

#include "interface/plugin.hpp"
#include "interface/plugin_so_interface.hpp"


#include "libconfig/libconfig.h++"

#include <limits>
#include <sstream>
#include <iostream>

#include <boost/scoped_array.hpp>

using namespace std;
using namespace theta;
using namespace theta::utils;
using namespace theta::plugin;

MultFunction::MultFunction(const ParIds & vids): Function(vids){
}

double MultFunction::operator()(const ParValues & v) const{
    double result = 1.0;
    for(ParIds::const_iterator it=par_ids.begin(); it!=par_ids.end(); it++){
        result *= v.get(*it);
    }
    return result;
}

/* DATA */
ObsIds Data::getObservables() const{
    ObsIds result;
    std::map<ObsId, Histogram>::const_iterator it = data.begin();
    for(;it!=data.end(); it++){
        result.insert(it->first);
    }
    return result;
}

const Histogram & Data::getData(const ObsId & obs_id) const{
    std::map<ObsId, Histogram>::const_iterator it = data.find(obs_id);
    if(it==data.end()) throw NotFoundException("Data::getData(): invalid obs_id");
    return it->second;
}

void Data::addData(const ObsId & obs_id, const Histogram & dat){
    data[obs_id] = dat;
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

Data Model::samplePseudoData(Random & rnd, const ParValues & values) const{
   Data result;
   for (ObsIds::const_iterator it = observables.begin(); it != observables.end(); it++) {
       Histogram pred, h;
       get_prediction_randomized(rnd, pred, values, *it);
       pred.fill_with_pseudodata(h, rnd);
      result.addData(*it, h);
   }
   return result;
}

ParValues Model::sampleValues(Random & rnd) const{
    ParValues result(*vm);
    for(std::vector<boost::shared_ptr<Distribution> >::const_iterator d_it=priors.begin(); d_it!=priors.end(); d_it++){
        (*d_it)->sample(result, rnd, *vm);
    }
    for(ParIds::const_iterator p_it = parameters.begin(); p_it!=parameters.end(); p_it++){
        if(not result.contains(*p_it)) result.set(*p_it, vm->get_default(*p_it));
    }
    return result;
}


void Model::set_prediction(const ObsId & obs_id, boost::ptr_vector<Function> & coeffs_, boost::ptr_vector<HistogramFunction> & histos_, const std::vector<std::string> & component_names){
    observables.insert(obs_id);
    const size_t n = coeffs_.size();
    if(n!=coeffs_.size() or n!=component_names.size()) throw InvalidArgumentException("Model::setPrediction: number of histograms, coefficients and component names do not match!");
    if(histos[obs_id].get()!=0)
        throw InvalidArgumentException("Model::setPrediction: prediction already set for this observable!");
    histos[obs_id].reset(new boost::ptr_vector<HistogramFunction>());
    histos[obs_id]->transfer(histos[obs_id]->end(), histos_.begin(), histos_.end(), histos_);
    
    if(coeffs[obs_id].get()!=0)
        throw InvalidArgumentException("Model::setPrediction: prediction already set for this observable (coeffs)!");
    coeffs[obs_id].reset(new boost::ptr_vector<Function>());
    coeffs[obs_id]->transfer(coeffs[obs_id]->end(), coeffs_.begin(), coeffs_.end(), coeffs_);
    names[obs_id] = component_names;
    
    for(boost::ptr_vector<Function>::const_iterator it=coeffs[obs_id]->begin(); it!=coeffs[obs_id]->end(); ++it){
        ParIds pids = (*it).getParameters();
        parameters.insert(pids.begin(), pids.end());
    }
    
    for(boost::ptr_vector<HistogramFunction>::const_iterator it=histos[obs_id]->begin(); it!=histos[obs_id]->end(); ++it){
        ParIds pids = (*it).getParameters();
        parameters.insert(pids.begin(), pids.end());
    }
}

void Model::get_prediction(Histogram & result, const ParValues & parameters, const ObsId & obs_id) const {
    histos_type::const_iterator it = histos.find(obs_id);
    if (it == histos.end()) throw InvalidArgumentException("Model::getPrediction: invalid obs_id");
    const histos_type::mapped_type & h_producers = it->second;
    coeffs_type::const_iterator it2 = coeffs.find(obs_id);
    const coeffs_type::mapped_type & h_coeffs = it2->second;
    bool result_init = false;
    for (size_t i = 0; i < h_producers->size(); i++) {
        if(result_init){
            result.add_with_coeff((*h_coeffs)[i](parameters), (*h_producers)[i](parameters));
        }
        else{
            result = (*h_producers)[i](parameters);
            result *= (*h_coeffs)[i](parameters);
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
    const histos_type::mapped_type & h_producers = it->second;
    coeffs_type::const_iterator it2 = coeffs.find(obs_id);
    const coeffs_type::mapped_type & h_coeffs = it2->second;
    bool result_init = false;
    for (size_t i = 0; i < h_producers->size(); i++) {
        if(result_init){
            result.add_with_coeff((*h_coeffs)[i](parameters), (*h_producers)[i].getRandomFluctuation(rnd, parameters));
        }
        else{
            result = (*h_producers)[i].getRandomFluctuation(rnd, parameters);
            result *= (*h_coeffs)[i](parameters);
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
    const histos_type::mapped_type & h_producers = it->second;
    if(i>=h_producers->size()) throw NotFoundException("Model::getPredictionComponent: parameter i out of bounds");
    coeffs_type::const_iterator it2 = coeffs.find(obs_id);
    const coeffs_type::mapped_type & h_coeffs = it2->second;
     
    Histogram result = (*h_producers)[i](parameters);
    result *= (*h_coeffs)[i](parameters);
    return result;
}

void Model::addPrior(auto_ptr<Distribution> & d){
    ParIds par_ids = d->getParameters();
    for(size_t i=0; i<priors.size(); ++i){
        for(ParIds::const_iterator par=par_ids.begin(); par!=par_ids.end(); ++par){
            if(priors[i]->getParameters().contains(*par)){
                throw InvalidArgumentException("Model::addPrior: trying to define two priors for one parameter!");
            }
        }
    }
    priors.push_back(boost::shared_ptr<Distribution>(d.release()));
}

const Distribution & Model::getPrior(size_t i) const{
    return *(priors[i]);
}

size_t Model::getNPriors() const{
    return priors.size();
}

NLLikelihood Model::getNLLikelihood(const Data & data) const{
    if(not(data.getObservables()==observables)){
        throw InvalidArgumentException("Model::createNLLikelihood: observables of model and data mismatch!");
    }
    NLLikelihood result(vm, *this, data, observables, parameters);
    //to test that everything is set up (i.e. no missing parameters or observable histograms in the model)
    // test the likelihood:
    ParValues pv(*vm);
    for(ParIds::const_iterator it=parameters.begin(); it!=parameters.end(); ++it){
        pv.set(*it, vm->get_default(*it));
    }
    result(pv); // will throw exception if parameters are missing ...
    return result;
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
            //the coefficients:
            size_t n_coeff = obs_setting[i]["coefficients"].size();
            if (n_coeff == 0) {
                stringstream ss;
                ss << "observable " << obs_name << ", component " << obs_setting[i].getName() << ", defined at path "
                        << obs_setting[i]["coefficients"].getPath() << " has wrong length (use a list!)";
                throw ConfigurationException(ss.str());
            }
            ParIds coeff_factors;
            try {
                for (size_t j = 0; j < n_coeff; j++) {
                    coeff_factors.insert(ctx.vm->getParId(obs_setting[i]["coefficients"][j]));
                }
            } catch (NotFoundException & e) {
                stringstream ss;
                ss << "while parsing coefficients for observable " << obs_name << ", component " << obs_setting[i].getName() << ", defined at path "
                        << obs_setting[i]["coefficients"].getPath() << ": NotFoundException occured: " << e.message;
                throw ConfigurationException(ss.str());
            }
            histos.push_back(hf);
            //hf should have lost pointer ownership:
            assert(hf.get() == 0);
            coeffs.push_back(new MultFunction(coeff_factors));
            names.push_back(obs_setting[i].getName());
        }
        result->set_prediction(*obsit, coeffs, histos, names);
    }
    //priors:
    if (s.exists("constraints")) {
        size_t n = s["constraints"].size();
        for (size_t i = 0; i < n; i++) {
            Configuration ctx2(ctx, s["constraints"][i]);
            std::auto_ptr<Distribution> d = PluginManager<Distribution>::build(ctx2);
            result->addPrior(d);
        }
    }
    return result;
}


/* NLLIKELIHOOD */
NLLikelihood::NLLikelihood(const boost::shared_ptr<VarIdManager> & vm_, const Model & m, const Data & dat, const ObsIds & obs, const ParIds & pars): Function(pars), vm(vm_), model(m),
        data(dat), obs_ids(obs), values(*vm){
}

double NLLikelihood::operator()(const ParValues & values) const{
    //check ranges:
    for(ParIds::const_iterator p_it = par_ids.begin(); p_it != par_ids.end(); ++p_it){
        const pair<double, double> & range = vm->get_range(*p_it);
        double value = values.get(*p_it);
        if(value < range.first || value > range.second)
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
        const Histogram & data_hist = data.getData(obs_id);
        const size_t nbins = model_prediction.get_nbins();
        const double * __restrict pred_data = model_prediction.getData();
        const double * __restrict data_data = data_hist.getData();
        for(size_t i=1; i<=nbins; i++){
             if(pred_data[i]>0.0)
                 result -= data_data[i] * theta::utils::log(pred_data[i]);
        }
        result += model_prediction.get_sum_of_bincontents();
    }
    if(isnan(result)){
        throw MathException("NLLikelihood::operator() would return nan (1)");
    }
    //the model prior:
    for(size_t i=0; i<model.priors.size(); i++){
        result += model.priors[i]->evalNL(values);
    }
    if(isnan(result)){
        throw MathException("NLLikelihood::operator() would return nan (2)");
    }
    return result;
}

