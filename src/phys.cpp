#include "interface/phys.hpp"
#include "interface/exception.hpp"
#include "interface/utils.hpp"
#include "interface/cfg-utils.hpp"
#include "interface/histogram-function.hpp"

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

double MultFunction::gradient(const ParValues & v, const ParId & pid) const{
    if(not par_ids.contains(pid)) return 0.0;
    double result = 1.0;
    for(ParIds::const_iterator it=par_ids.begin(); it!=par_ids.end() && *it!=pid; it++){
        result *= v.get(*it);
    }
    return result;
}

/*Function * MultFunction::clone() const{
    return new MultFunction(v_ids);
}*/

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
Model::Model(boost::shared_ptr<VarIdManager> & vm_, const ParIds & parameter_ids, const ObsIds & observable_ids):
                   vm(vm_), parameters(parameter_ids), observables(observable_ids){
}

ParIds Model::getParameters() const{
    return parameters;
}

ObsIds Model::getObservables() const{
    return observables;
}

void Model::setPrediction(const ObsId & obs_id, boost::ptr_vector<Function> & coeffs_, boost::ptr_vector<HistogramFunction> & histos_, const std::vector<std::string> & component_names){
    if(!observables.contains(obs_id)) throw InvalidArgumentException("Model::setPrediction: used an invalid obs_id!");
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

    //coeffs[obs_id].transfer(coeffs[obs_id].end(), coeffs_.begin(), coeffs_.end(), coeffs_);
    names[obs_id] = component_names;
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

void Model::get_prediction_randomized(AbsRandomProxy & rnd, Histogram &result, const ParValues & parameters, const ObsId & obs_id) const{
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

void Model::get_prediction_derivative(Histogram & result, const ParValues & parameters, const ObsId & obs_id, const ParId & pid) const{
    histos_type::const_iterator it = histos.find(obs_id);
    if (it == histos.end()) throw InvalidArgumentException("Model::get_prediction_derivative: invalid obs_id");
    const histos_type::mapped_type & h_producers = it->second;
    coeffs_type::const_iterator it2 = coeffs.find(obs_id);
    const coeffs_type::mapped_type & h_coeffs = it2->second;

    //Histogram & result = predictions_d[obs_id];
    //save whether result has already been initialized:
    bool result_init = false;
    for (size_t i = 0; i < h_producers->size(); i++) {
        if((*h_producers)[i].dependsOn(pid)){
            if(result_init){
                result.add_with_coeff((*h_coeffs)[i](parameters), (*h_producers)[i].gradient(parameters, pid));
            }
            else{
                result = (*h_producers)[i].gradient(parameters, pid);
                result *= (*h_coeffs)[i](parameters);
                result_init = true;
            }
        }
        double coeff_g = (*h_coeffs)[i].gradient(parameters, pid);
        if(coeff_g!=0.0){
            if(result_init){
                result.add_with_coeff(coeff_g, (*h_producers)[i](parameters));
            }
            else{
                result = (*h_producers)[i](parameters);
                result *= coeff_g;
                result_init = true;
            }
        }
    }
    if(not result_init){
        const pair<double, double> & o_range = vm->get_range(obs_id);
        result.reset(vm->get_nbins(obs_id), o_range.first, o_range.second);
    }
    //return result;
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
    NLLikelihood result(*this, data, observables, parameters);
    //to test that everything is set up (i.e. no missing parameters or observable histograms in the model)
    // test the likelihood:
    //boost::scoped_array<double> pars(new double[result.getnpar()]);
    ParValues pv(*vm);
    for(ParIds::const_iterator it=parameters.begin(); it!=parameters.end(); ++it){
        pv.set(*it, vm->get_default(*it));
    }
    result(pv); // will throw exception if parameters are missing ...
    return result;
}

NLLikelihood Model::getNLLikelihood(const Data & data, const ParIds & pars, const ObsIds & obs_ids) const{
    //check that each observable specified by obs_ids is both (a) described by this model and (b) present in the data.
    ObsIds data_observables = data.getObservables();
    for(ObsIds::const_iterator obsit=obs_ids.begin(); obsit!=obs_ids.end(); obsit++){
        if(!observables.contains(*obsit))
            throw InvalidArgumentException("Model::createNLLikelihood: observable specified which is not modelled");
         if(!data_observables.contains(*obsit))
             throw InvalidArgumentException("Model::createNLLikelihood: observable specified which is not present in data");
    }

    for(ParIds::const_iterator parit=pars.begin(); parit!=pars.end(); parit++){
        if(!parameters.contains(*parit))
            throw InvalidArgumentException("Model::createNLLikelihood: parameter specified which is not modelled");
    }
    NLLikelihood result =  NLLikelihood(*this, data, obs_ids, pars);
    ParValues pv(*vm);
    for(ParIds::const_iterator it=parameters.begin(); it!=parameters.end(); ++it){
        pv.set(*it, vm->get_default(*it));
    }
    result(pv);
    return result;
}


/* ModelFactory */
std::auto_ptr<Model> ModelFactory::buildModel(ConfigurationContext & ctx) {
    const libconfig::Setting & s = ctx.setting;
    //VarIdManager & vm = *ctx.vm;
    //get the observables:
    ObsIds observables;
    int nobs = s["observables"].getLength();
    if (nobs == 0)
        throw ConfigurationException("model has no observables.");
    for (int i = 0; i < nobs; i++) {
        string obs_name = s["observables"][i].getName();
        double min = get_double_or_inf(s["observables"][i]["range"][0]);
        double max = get_double_or_inf(s["observables"][i]["range"][1]);
        int nbins = s["observables"][i]["nbins"];
        s["observables"][i].remove("range");
        s["observables"][i].remove("nbins");
        if (min >= max) {
            stringstream ss;
            ss << "observable " << obs_name << " has empty range.";
            throw ConfigurationException(ss.str());
        }
        if (nbins <= 0) {
            stringstream ss;
            ss << "observable " << obs_name << " hasnbins<=0.";
            throw ConfigurationException(ss.str());
        }
        observables.insert(ctx.vm->createObsId(obs_name, (size_t) nbins, min, max));
    }
    //get parameters:
    ParIds parameters;
    int npar = s["parameters"].getLength();
    if (npar == 0)
        throw ConfigurationException("model has no parameters.");
    for (int i = 0; i < npar; i++) {
        string par_name = s["parameters"][i].getName();
        double def = s["parameters"][i]["default"];
        double min = get_double_or_inf(s["parameters"][i]["range"][0]);
        double max = get_double_or_inf(s["parameters"][i]["range"][1]);
        s["parameters"][i].remove("default");
        s["parameters"][i].remove("range");
        if (min >= max) {
            stringstream ss;
            ss << "parameter " << par_name << " has empty range.";
            throw ConfigurationException(ss.str());
        }
        if (def < min || def > max) {
            stringstream ss;
            ss << "parameter " << par_name << " has default value outside of its range.";
            throw ConfigurationException(ss.str());
        }
        parameters.insert(ctx.vm->createParId(par_name, def, min, max));
    }
    std::auto_ptr<Model> result(new Model(ctx.vm, parameters, observables));
    //go through observables to find the template definition for each of them:
    for (ObsIds::const_iterator obsit = observables.begin(); obsit != observables.end(); obsit++) {
        string obs_name = ctx.vm->getName(*obsit);
        libconfig::Setting & obs_setting = s[obs_name];
        boost::ptr_vector<HistogramFunction> histos;
        boost::ptr_vector<Function> coeffs;
        vector<string> names;
        for (int i = 0; i < obs_setting.getLength(); i++) {
            ConfigurationContext context(ctx, obs_setting[i]["histogram"]);
            auto_ptr<HistogramFunction> hf = PluginManager<HistogramFunctionFactory>::get_instance()->build(context);
            //the coefficients:
            int n_coeff = obs_setting[i]["coefficients"].getLength();
            if (n_coeff == 0) {
                stringstream ss;
                ss << "observable " << obs_name << ", component " << obs_setting[i].getName() << ", defined on line "
                        << obs_setting[i]["coefficients"].getSourceLine() << " has wrong type (use a list!)";
                throw ConfigurationException(ss.str());
            }
            ParIds coeff_factors;
            try {
                for (int j = 0; j < n_coeff; j++) {
                    coeff_factors.insert(ctx.vm->getParId(obs_setting[i]["coefficients"][j]));
                }
            } catch (NotFoundException & e) {
                stringstream ss;
                ss << "while parsing coefficients for observable " << obs_name << ", component " << obs_setting[i].getName() << ", defined on line "
                        << obs_setting[i]["coefficients"].getSourceLine() << ": NotFoundException occured: " << e.message;
                throw ConfigurationException(ss.str());
            }
            obs_setting[i].remove("coefficients");
            histos.push_back(hf);
            //hf should have lost pointer ownership:
            assert(hf.get() == 0);
            coeffs.push_back(new MultFunction(coeff_factors));
            names.push_back(obs_setting[i].getName());
        }
        result->setPrediction(*obsit, coeffs, histos, names);
    }
    //priors:
    if (s.exists("constraints")) {
        int n = s["constraints"].getLength();
        for (int i = 0; i < n; i++) {
            ConfigurationContext ctx2(ctx, s["constraints"][i]);
            std::auto_ptr<Distribution> d = PluginManager<DistributionFactory>::get_instance()->build(ctx2);
            result->addPrior(d);
        }
    }
    return result;
}


/* NLLIKELIHOOD */
NLLikelihood::NLLikelihood(const Model & m, const Data & dat, const ObsIds & obs, const ParIds & pars): Function(pars), model(m),
        data(dat), obs_ids(obs), values(model.getVarIdManager()), p_derivatives(model.getVarIdManager()){
}

double NLLikelihood::operator()(const ParValues & values) const{
    double result = 0.0;
    //go through all observables:
    for(ObsIds::const_iterator obsit=obs_ids.begin(); obsit!=obs_ids.end(); obsit++){
        const ObsId & obs_id = *obsit;
        Histogram model_prediction;
        try{
            model.get_prediction(model_prediction, values, obs_id);
        }catch(Exception & ex){
            ex.message += " (in NLLikelihood::operator() model.get_prediction()): ";
            ParIds ids = values.getAllParIds();
            for(ParIds::const_iterator it=ids.begin(); it!=ids.end(); ++it){
                ex.message += model.getVarIdManager().getName(*it) + "=";
                ex.message += values.get(*it);
                ex.message += "; ";
            }
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
        result += model_prediction.get_sum_of_weights();
    }
    if(isnan(result)){
        throw MathException("NLLikelihood:Loperator() would return nan (1)");
    }
    //the model prior:
    for(size_t i=0; i<model.priors.size(); i++){
        result += model.priors[i]->evalNL(values);
    }
    if(isnan(result)){
        throw MathException("NLLikelihood:Loperator() would return nan (2)");
    }
    return result;
}

void NLLikelihood::gradient(const double* x, double* g, double & nll) const{
    nll = Function::operator()(x);
    boost::scoped_array<double> xnew(new double[getnpar()]);
    memcpy(xnew.get(), x, getnpar() * sizeof(double));
    for(size_t i=0; i<getnpar(); i++){
        double epsilon = 1e-8 * fabs(x[i]);
        if(epsilon == 0.0)epsilon = 1e-8;
        xnew[i] = x[i] + epsilon;
        double nll_plus = Function::operator()(xnew.get());
        xnew[i] = x[i] - epsilon;
        double nll_minus = Function::operator()(xnew.get());
        xnew[i] = x[i];
        g[i] = (nll_plus - nll_minus) / (2*epsilon);
    }
}

void NLLikelihood::gradient2(const double* x, double* g, double & result) const{
    size_t i=0;
    for(ParIds::const_iterator parit=par_ids.begin(); parit!=par_ids.end(); parit++, i++){
        values.set(*parit, x[i]);
        g[i] = 0.0;
    }
    
    result = 0.0;
    //go through all observables:
    for(ObsIds::const_iterator obsit=obs_ids.begin(); obsit!=obs_ids.end(); obsit++){
        const ObsId & obs_id = *obsit;
        Histogram & model_prediction = predictions[obs_id];
        model.get_prediction(model_prediction, values, obs_id);
        const Histogram & data_hist = data.getData(obs_id);
        const size_t nbins = model_prediction.get_nbins();
        const double * pred_data = model_prediction.getData();
        const double * data_data = data_hist.getData();
        for(size_t k=1; k<=nbins; k++){
             if(pred_data[k]>0.0)
                 result -= data_data[k] * theta::utils::log(pred_data[k]);
        }
        result += model_prediction.get_sum_of_weights();
        
        //derivatives:
        i=0;
        for(ParIds::const_iterator parit=par_ids.begin(); parit!=par_ids.end(); parit++, i++){
            Histogram & model_prediction_d = predictions_d[obs_id];
            model.get_prediction_derivative(model_prediction_d, values, obs_id, *parit);
            const double * pred_d_data = model_prediction_d.getData();
            for(size_t k=1; k<=nbins; k++){
                 if(pred_data[k]>0.0)
                     g[i] -= data_data[k] * pred_d_data[k] / pred_data[k];
            }
            g[i] += model_prediction_d.get_sum_of_weights();
        }
    }
    
    //the model priors:
    for(size_t k=0; k<model.priors.size(); k++){
        result += model.priors[k]->evalNL_withDerivatives(values, p_derivatives);
        //write derivatives to g:
        i=0;
        for(ParIds::const_iterator parit=par_ids.begin(); parit!=par_ids.end(); parit++, i++){
            if(p_derivatives.contains(*parit))
                g[i] += p_derivatives.get(*parit);
        }
    }
}

double NLLikelihood::gradient(const ParValues & values, const ParId & pid) const{
    double der = 0.0;
    for(ObsIds::const_iterator obsit=obs_ids.begin(); obsit!=obs_ids.end(); obsit++){
        const ObsId & obs_id = *obsit;
        Histogram & model_prediction = predictions[obs_id];
        model.get_prediction(model_prediction, values, obs_id);
        const Histogram & data_hist = data.getData(obs_id);
        const size_t nbins = model_prediction.get_nbins();
        const double * pred_data = model_prediction.getData();
        const double * data_data = data_hist.getData();

        Histogram & model_prediction_d = predictions_d[obs_id];
        model.get_prediction_derivative(model_prediction_d, values, obs_id, pid);
        const double * pred_d_data = model_prediction_d.getData();
        for(size_t k=1; k<=nbins; k++){
             if(pred_data[k]>0.0)
                 der -= data_data[k] * pred_d_data[k] / pred_data[k];
        }
        der += model_prediction_d.get_sum_of_weights();
    }
    for(size_t k=0; k<model.priors.size(); k++){
        model.priors[k]->evalNL_withDerivatives(values, p_derivatives);
        if(p_derivatives.contains(pid))
               der += p_derivatives.get(pid);
    }
    return der;
}

