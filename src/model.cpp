#include "interface/model.hpp"

using namespace std;
using namespace theta;
using namespace theta::utils;
using namespace theta::plugin;

REGISTER_PLUGIN_BASETYPE(Model);

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
   Data pred;
   get_prediction_randomized(rnd, pred, values);
   for (ObsIds::const_iterator it = observables.begin(); it != observables.end(); it++) {
       pred[*it].fill_with_pseudodata(result[*it], rnd);
   }
}



/* default_model */
void default_model::set_prediction(const ObsId & obs_id, boost::ptr_vector<Function> & coeffs_, boost::ptr_vector<HistogramFunction> & histos_){
    observables.insert(obs_id);
    const size_t n = coeffs_.size();
    if(n!=coeffs_.size()) throw InvalidArgumentException("Model::setPrediction: number of histograms and coefficients do not match");
    if(histos[obs_id].size()>0)
        throw InvalidArgumentException("Model::setPrediction: prediction already set for this observable");
    histos[obs_id].transfer(histos[obs_id].end(), histos_.begin(), histos_.end(), histos_);
    if(coeffs[obs_id].size()>0)
        throw InvalidArgumentException("Model::setPrediction: prediction already set for this observable (coeffs)");
    coeffs[obs_id].transfer(coeffs[obs_id].end(), coeffs_.begin(), coeffs_.end(), coeffs_);
    for(boost::ptr_vector<Function>::const_iterator it=coeffs[obs_id].begin(); it!=coeffs[obs_id].end(); ++it){
        ParIds pids = (*it).getParameters();
        parameters.insert(pids.begin(), pids.end());
    }
    for(boost::ptr_vector<HistogramFunction>::const_iterator it=histos[obs_id].begin(); it!=histos[obs_id].end(); ++it){
        ParIds pids = (*it).getParameters();
        parameters.insert(pids.begin(), pids.end());
    }
}

void default_model::get_prediction(Data & result, const ParValues & parameters) const {
    for(ObsIds::const_iterator obsit=observables.begin(); obsit!=observables.end(); ++obsit){
        histos_type::const_iterator it = histos.find(*obsit);
        assert(it!=histos.end());
        histos_type::const_mapped_reference h_producers = *(it->second);
        coeffs_type::const_iterator it2 = coeffs.find(*obsit);
        coeffs_type::const_mapped_reference h_coeffs = *(it2->second);
        bool result_init = false;
        for (size_t i = 0; i < h_producers.size(); i++) {
            if(result_init){
                result[*obsit].add_with_coeff(h_coeffs[i](parameters), h_producers[i](parameters));
            }
            else{
                result[*obsit] = h_producers[i](parameters);
                result[*obsit] *= h_coeffs[i](parameters);
                result_init = true;
            }
        }
        //return empty prediction as correct zero template:
        if(not result_init){
            const pair<double, double> & o_range = vm->get_range(*obsit);
            result[*obsit].reset(vm->get_nbins(*obsit), o_range.first, o_range.second);
        }
    }
}

void default_model::get_prediction_randomized(Random & rnd, Data & result, const ParValues & parameters) const{
    for(ObsIds::const_iterator obsit=observables.begin(); obsit!=observables.end(); ++obsit){
        histos_type::const_iterator it = histos.find(*obsit);
        assert(it!=histos.end());
        histos_type::const_mapped_reference h_producers = *(it->second);
        coeffs_type::const_iterator it2 = coeffs.find(*obsit);
        coeffs_type::const_mapped_reference h_coeffs = *(it2->second);
        bool result_init = false;
        for (size_t i = 0; i < h_producers.size(); i++) {
            if(result_init){
                result[*obsit].add_with_coeff(h_coeffs[i](parameters), h_producers[i].getRandomFluctuation(rnd, parameters));
            }
            else{
                result[*obsit] = h_producers[i].getRandomFluctuation(rnd, parameters);
                result[*obsit] *= h_coeffs[i](parameters);
                result_init = true;
            }
        }
        if(not result_init){
            const pair<double, double> & o_range = vm->get_range(*obsit);
            result[*obsit].reset(vm->get_nbins(*obsit), o_range.first, o_range.second);
        }
    }
}

std::auto_ptr<NLLikelihood> default_model::getNLLikelihood(const Data & data) const{
    if(not(data.getObservables()==observables)){
        throw InvalidArgumentException("Model::createNLLikelihood: observables of model and data mismatch!");
    }
    return std::auto_ptr<NLLikelihood>(new default_model_nll(*this, data, observables));
}

default_model::default_model(const Configuration & ctx): Model(ctx.vm){
    SettingWrapper s = ctx.setting;
    ObsIds observables = ctx.vm->getAllObsIds();
    //go through observables to find the template definition for each of them:
    for (ObsIds::const_iterator obsit = observables.begin(); obsit != observables.end(); obsit++) {
        string obs_name = ctx.vm->getName(*obsit);
        if(not s.exists(obs_name)) continue;
        SettingWrapper obs_setting = s[obs_name];
        boost::ptr_vector<HistogramFunction> histos;
        boost::ptr_vector<Function> coeffs;
        for (size_t i = 0; i < obs_setting.size(); i++) {
            Configuration context(ctx, obs_setting[i]["histogram"]);
            auto_ptr<HistogramFunction> hf = PluginManager<HistogramFunction>::instance().build(context);
            Configuration cfg(ctx, obs_setting[i]["coefficient-function"]);
            auto_ptr<Function> coeff_function = PluginManager<Function>::instance().build(cfg);
            coeffs.push_back(coeff_function);
            assert(coeff_function.get()==0);
            histos.push_back(hf);
        }
        set_prediction(*obsit, coeffs, histos);
    }
    parameter_distribution = PluginManager<Distribution>::instance().build(Configuration(ctx, s["parameter-distribution"]));
    if(not (parameter_distribution->getParameters() == getParameters())){
        stringstream ss;
        ss << "parameter-distribution does not define exactly the model parameters dist = ( ";
        ParIds dist_pars = parameter_distribution->getParameters();
        ParIds m_pars = getParameters();
        for(ParIds::const_iterator p_it=dist_pars.begin(); p_it!=dist_pars.end(); ++p_it){
            ss << ctx.vm->getName(*p_it) << " ";
        }
        ss << "); model = ( ";
        for(ParIds::const_iterator p_it=m_pars.begin(); p_it!=m_pars.end(); ++p_it){
            ss << ctx.vm->getName(*p_it) << " ";
        }
        ss << " )";
        throw ConfigurationException(ss.str());
    }
}


/* default_model_nll */
default_model_nll::default_model_nll(const default_model & m, const Data & dat, const ObsIds & obs): model(m),
        data(dat), obs_ids(obs), additional_terms(0), override_distribution(0){
    Function::par_ids = model.getParameters();
    for(ParIds::const_iterator it=par_ids.begin(); it!=par_ids.end(); ++it){
        ranges[*it] = model.get_parameter_distribution().support(*it);
    }
}

void default_model_nll::set_additional_terms(const boost::ptr_vector<Function> * terms){
    additional_terms = terms;
}

void default_model_nll::set_override_distribution(const Distribution * d){
    override_distribution = d;
    if(d){
        for(ParIds::const_iterator it=par_ids.begin(); it!=par_ids.end(); ++it){
            ranges[*it] = d->support(*it);
        }
    }else{
        for(ParIds::const_iterator it=par_ids.begin(); it!=par_ids.end(); ++it){
            ranges[*it] = model.get_parameter_distribution().support(*it);
        }
    }
}


double default_model_nll::operator()(const ParValues & values) const{
    double result = 0.0;
    //1. the model prior first, because if we are out of bounds, we should not evaluate
    //   the likelihood of the templates ...
    if(override_distribution){
        result += override_distribution->evalNL(values);
    }
    else{
        result += model.get_parameter_distribution().evalNL(values);
    }
    //2. get the prediciton of the model:
    try{
        model.get_prediction(predictions, values);
    }
    catch(Exception & ex){
         ex.message += " (in NLLikelihood::operator() calling model.get_prediction())";
        throw;
    }
    //3. the template likelihood
    for(ObsIds::const_iterator obsit=obs_ids.begin(); obsit!=obs_ids.end(); obsit++){
        const ObsId & obs_id = *obsit;
        Histogram & model_prediction = predictions[obs_id];
        const Histogram & data_hist = data[obs_id];
        const size_t nbins = data_hist.get_nbins();
        assert(data_hist.get_nbins() == model_prediction.get_nbins());
        const double * __restrict pred_data = model_prediction.getData();
        const double * __restrict data_data = data_hist.getData();
        double nll = 0.0;
        for(size_t i=1; i<=nbins; ++i){
            //if both, the prediction and the data are zero, that does not contribute.
            // However, if the prediction is zero and we have non-zero data, we MUST return infinity (!) ...
            if(pred_data[i] > 0.0)
                 nll -= data_data[i] * theta::utils::log(pred_data[i]);
             else if(data_data[i] > 0.0){
                 return numeric_limits<double>::infinity();
             }
        }
        result += nll;
        result += model_prediction.get_sum_of_bincontents();
    }
    
    //3. The additional likelihood terms, if set:
    if(additional_terms){
        for(size_t i=0; i < additional_terms->size(); i++){
            result += ((*additional_terms)[i])(values);
        }
    }
    return result;
}

REGISTER_PLUGIN_DEFAULT(default_model)

