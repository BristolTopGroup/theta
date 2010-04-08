#include "core.hpp"
#include "plugins/interpolating-histogram.hpp"
#include "interface/random.hpp"
#include "interface/run.hpp"

#include <iostream>
#include <boost/scoped_array.hpp>

using namespace theta;
using namespace theta::plugin;
using namespace libconfig;
using namespace std;

fixed_poly::fixed_poly(const Configuration & ctx){
    SettingWrapper s = ctx.setting;
    ObsId obs_id = ctx.vm->getObsId(s["observable"]);
    int order = s["coefficients"].size() - 1;
    if (order == -1) {
        stringstream ss;
        ss << "Empty definition of coefficients for polynomial at path " << s["coefficients"].getPath();
        throw ConfigurationException(ss.str());
    }
    size_t nbins = ctx.vm->get_nbins(obs_id);
    pair<double, double> range = ctx.vm->get_range(obs_id);
    Histogram h(nbins, range.first, range.second);
    vector<double> coeffs(order + 1);
    for (int i = 0; i <= order; i++) {
        coeffs[i] = s["coefficients"][i];
    }
    for (size_t i = 1; i <= nbins; i++) {
        double x = h.get_bincenter(i);
        double value = coeffs[order];
        for (int k = 0; k < order; k++) {
            value *= x;
            value += coeffs[order - k - 1];
        }
        h.set(i, value);
    }
    double norm_to = s["normalize_to"];
    double norm;
    if ((norm = h.get_sum_of_bincontents()) == 0.0) {
        throw ConfigurationException("Histogram specification is zero (can't normalize)");
    }
    h *= norm_to / norm;
    set_histo(h);
}

fixed_gauss::fixed_gauss(const Configuration & ctx){
    SettingWrapper s = ctx.setting;
    double width = s["width"];
    double mean = s["mean"];
    ObsId obs_id = ctx.vm->getObsId(s["observable"]);
    size_t nbins = ctx.vm->get_nbins(obs_id);
    pair<double, double> range = ctx.vm->get_range(obs_id);
    Histogram h(nbins, range.first, range.second);
    //fill the histogram:
    for (size_t i = 1; i <= nbins; i++) {
        double d = (h.get_bincenter(i) - mean) / width;
        h.set(i, exp(-0.5 * d * d));
    }
    double norm_to = s["normalize_to"];
    double norm;
    if ((norm = h.get_sum_of_bincontents()) == 0.0) {
        throw ConfigurationException("Histogram specification is zero (can't normalize)");
    }
    h *= norm_to / norm;
    set_histo(h);
}

log_normal::log_normal(const Configuration & cfg){
    support_.first = 0.0;
    support_.second = std::numeric_limits<double>::infinity();
    SettingWrapper s = cfg.setting;
    mu = s["mu"];
    sigma = s["sigma"];
    if(sigma <= 0.0){
        throw ConfigurationException("log_normal: sigma <= 0.0 is not allowed");
    }
    string par_name = s["parameter"];
    par_ids.insert(cfg.vm->getParId(par_name));
}

double log_normal::evalNL(const ParValues & values) const {
    double x = values.get(*par_ids.begin());
    if (x <= 0.0) return numeric_limits<double>::infinity();
    double tmp = (log(x) - mu) / sigma;
    return 0.5 * tmp * tmp + log(x);
}

double log_normal::evalNL_withDerivatives(const ParValues & values, ParValues & derivatives) const{
    const ParId & pid = *par_ids.begin();
    double x = values.get(pid);
    if (x <= 0.0){
        derivatives.set(pid, 0.0);
        return numeric_limits<double>::infinity();
    }
    double tmp = (theta::utils::log(x) - mu) / sigma;
    derivatives.set(pid, 1/x*(1.0 + tmp));
    return 0.5 * tmp * tmp + log(x);
}

void log_normal::sample(ParValues & result, Random & rnd) const {
    const ParId & pid = *par_ids.begin();
    double value = exp(rnd.gauss(sigma) + mu);
    result.set(pid, value);
}

const std::pair<double,double> & log_normal::support(const theta::ParId & p) const{
    return support_;
}

double log_normal::width(const theta::ParId &) const{
    return sqrt((exp(sigma * sigma) - 1)*exp(2*mu + sigma*sigma));
}

void log_normal::mode(theta::ParValues & result) const{
    result.set(*(par_ids.begin()), exp(mu - sigma*sigma));
}

/* START delta_distribution */
delta_distribution::delta_distribution(const theta::plugin::Configuration & cfg){
    for(size_t i=0; i<cfg.setting.size(); ++i){
        if(cfg.setting[i].getName()=="type") continue;
        const ParId pid = cfg.vm->getParId(cfg.setting[i].getName());
        const double val = cfg.setting[i];
        values.set(pid, val);
        supports[pid].second = supports[pid].first = val;
    }
    par_ids = values.getAllParIds();
}

void delta_distribution::sample(theta::ParValues & result, theta::Random &) const{
    result.set(values);
}

void delta_distribution::mode(theta::ParValues & result) const{
    result.set(values);
}

double delta_distribution::evalNL(const theta::ParValues & vals) const{
    return 0.0;
}

double delta_distribution::evalNL_withDerivatives(const theta::ParValues & values, theta::ParValues & derivatives) const{
    for(ParIds::const_iterator it=par_ids.begin(); it!=par_ids.end(); ++it){
        derivatives.set(*it, 0.0);
    }
    return 0.0;
}

const std::pair<double, double> & delta_distribution::support(const theta::ParId& p) const{
    std::map<theta::ParId, std::pair<double, double> >::const_iterator it=supports.find(p);
    if(it==supports.end()) throw InvalidArgumentException("delta_distribution::support: parameter not found");
    return it->second;
}

double delta_distribution::width(const theta::ParId &) const{
    return 0.0;
}

/* START flat_distribution */

flat_distribution::flat_distribution(const theta::plugin::Configuration & cfg){
    for(size_t i=0; i<cfg.setting.size(); ++i){
        if(cfg.setting[i].getName()=="type") continue;
        SettingWrapper s = cfg.setting[i];
        ParId pid = cfg.vm->getParId(s.getName());
        par_ids.insert(pid);
        double low = ranges[pid].first = s["range"][0].get_double_or_inf();
        double high = ranges[pid].second = s["range"][1].get_double_or_inf();
        if(low > high) throw ConfigurationException("invalid range");
        if(s.exists("width")){
            widths.set(pid, s["width"]);
        }
        else{
            if(not std::isinf(high-low))
                widths.set(pid, 0.1 * (high - low));
            //otherwise: not set and width(...) will throw ...
        }
        modes.set(pid, 0.5 * (high + low));
        if(s.exists("fix-sample-value")){
            fix_sample_values.set(pid, s["fix-sample-value"]);
            modes.set(pid, fix_sample_values.get(pid));
            if(not widths.contains(pid) && fix_sample_values.get(pid) > 0.0)
                widths.set(pid, fix_sample_values.get(pid)*0.1);
        }
    }
}

void flat_distribution::sample(theta::ParValues & result, theta::Random & rnd) const{
    result.set(fix_sample_values);
    for(std::map<theta::ParId, std::pair<double, double> >::const_iterator it = ranges.begin(); it!=ranges.end(); ++it){
        if(fix_sample_values.contains(it->first))continue;
        const double low = it->second.first;
        const double high = it->second.second;
        if(std::isinf(high - low)){
            throw IllegalStateException("flat_distribution::sample: infinite range and no fix-sample-value");
        }
        result.set(it->first, rnd.uniform()*(high-low) + low);
    }
}

void flat_distribution::mode(theta::ParValues & result) const{
    result.set(modes);
}

double flat_distribution::evalNL(const theta::ParValues & values) const{
    for(std::map<theta::ParId, std::pair<double, double> >::const_iterator it = ranges.begin(); it!=ranges.end(); ++it){
        const double val = values.get(it->first);
        if(it->second.first > val || it->second.second < val) return numeric_limits<double>::infinity();
    }
    return 0.0;
}

double flat_distribution::evalNL_withDerivatives(const theta::ParValues & values, theta::ParValues & derivatives) const{
    //for(ParIds::const_iterator it=par_ids.begin(); it!=par_ids.end(); ++it){
    for(std::map<theta::ParId, std::pair<double, double> >::const_iterator it = ranges.begin(); it!=ranges.end(); ++it){
        const double val = values.get(it->first);
        derivatives.set(it->first, 0.0);
        if(it->second.first > val || it->second.second < val) return numeric_limits<double>::infinity();
    }
    return 0.0;
}

const std::pair<double, double> & flat_distribution::support(const theta::ParId& p) const{
    std::map<theta::ParId, std::pair<double, double> >::const_iterator it=ranges.find(p);
    if(it==ranges.end()) throw InvalidArgumentException("flat_distribution::support: parameter not found");
    return it->second;
}

double flat_distribution::width(const theta::ParId & p) const{
    return widths.get(p); //throws NotFoundException if not set
}


/* START gauss */
void gauss::sample(ParValues & result, Random & rnd) const{
    const size_t n = v_par_ids.size();
    //TODO: requires allocation every time. That could be moved as mutable to the class
    // to save this allocations.
    boost::scoped_array<double> x(new double[n]);
    boost::scoped_array<double> x_trafo(new double[n]);
    int rep = 0;
    bool repeat;
    do{
        repeat = false;
        for(size_t i=0; i<n; i++){
            x[i] = rnd.gauss();
            x_trafo[i] = 0.0;
        }
        //transform x by sqrt_cov:
        for(size_t i=0; i<n; i++){
            for(size_t j=0; j<=i; j++){//sqrt_cov is lower triangular
                x_trafo[i] += sqrt_cov(i,j) * x[j];
            }
        }
        size_t i=0;
        for(vector<ParId>::const_iterator v=v_par_ids.begin(); v!=v_par_ids.end(); ++v, ++i){
            const pair<double, double> & range = ranges[i];
            double value;
            if(range.first == range.second){
                value = range.first;
            }
            else{
                value = x_trafo[i] + mu[i];
            }
            if(value > range.second || value < range.first){
                repeat = true;
                break;
            }
            result.set(*v, value);
        }
        rep++;
        if(rep==100000) throw Exception("gauss::sample: too many iterations necessary to respect bounds!");
    }while(repeat);
}

double gauss::evalNL(const ParValues & values) const{
    const size_t n = v_par_ids.size();
    boost::scoped_array<double> delta(new double[n]);
    size_t i=0;
    for(vector<ParId>::const_iterator v=v_par_ids.begin(); v!=v_par_ids.end(); v++){
        delta[i] = values.get(*v) - mu[i];
        i++;
    }
    //compute 0.5 * delta^T * inverse_cov * delta, where inverse_cov is symmetric:
    double e = 0.0;
    for(size_t i=0; i<n; i++){
        const double delta_i = delta[i];
        for(size_t j=0; j<i; j++){
            e += delta_i * inverse_cov(i,j) * delta[j];
        }
        e += 0.5 * delta_i * delta_i * inverse_cov(i,i);
    }
    return e;
}

double gauss::evalNL_withDerivatives(const ParValues & values, ParValues & derivatives) const{
    const size_t n = v_par_ids.size();
    boost::scoped_array<double> delta(new double[n]);
    size_t i=0;
    for(vector<ParId>::const_iterator v=v_par_ids.begin(); v!=v_par_ids.end(); v++){
        delta[i] = values.get(*v) - mu[i];
        i++;
        derivatives.set(*v, 0.0);
    }
    //compute delta^T * inverse_cov * delta:
    double e = 0.0;
    for(size_t i=0; i<n; i++){
        const double delta_i = delta[i];
        for(size_t j=0; j<i; j++){
            e += delta_i * inverse_cov(i,j) * delta[j];
            derivatives.addTo(v_par_ids[i], inverse_cov(i,j) * delta[j]);
            derivatives.addTo(v_par_ids[j], inverse_cov(i,j) * delta[i]);
        }
        e += 0.5 * delta_i * delta_i * inverse_cov(i,i);
        derivatives.addTo(v_par_ids[i], inverse_cov(i,i) * delta[i]);
    }    
    return e;
}

gauss::gauss(const Configuration & cfg){
    Matrix cov;
      if(cfg.setting.exists("parameter")){
            mu.resize(1);
            cov.reset(1,1);
            ranges.resize(1);
            v_par_ids.push_back(cfg.vm->getParId(cfg.setting["parameter"]));
            mu[0] = cfg.setting["mean"];
            double width = cfg.setting["width"];
            cov(0,0) = width*width;
            ranges[0].first = cfg.setting["range"][0].get_double_or_inf();
            ranges[0].second = cfg.setting["range"][1].get_double_or_inf();
        }
        else{ //multi-dimensional case:
           size_t n = cfg.setting["parameters"].size();
           if(n==0){
               stringstream ss;
               ss << "While building gauss distribution defined at path " << cfg.setting.getPath() << ": expected one or more 'parameters'.";
               throw ConfigurationException(ss.str());
           }
           if(cfg.setting["ranges"].size()!=n || cfg.setting["mu"].size()!=n || cfg.setting["covariance"].size()!=n){
               throw ConfigurationException("gauss: length of ranges, mu, covariance mismatch!");
           }
           mu.resize(n);
           cov.reset(n,n);
           ranges.resize(n);
           for(size_t i=0; i<n; i++){
               v_par_ids.push_back(cfg.vm->getParId(cfg.setting["parameters"][i]));
               mu[i] = cfg.setting["mean"][i];
               ranges[i].first = cfg.setting["ranges"][i][0];
               ranges[i].second = cfg.setting["ranges"][i][1];
               for(size_t j=0; j<n; j++){
                   cov(i,j) = cfg.setting["covariance"][i][j];
               }
           }
      }
    for(vector<ParId>::const_iterator p_it=v_par_ids.begin(); p_it!=v_par_ids.end(); p_it++){
        par_ids.insert(*p_it);
    }
    sqrt_cov = cov;
    inverse_cov = cov;
    sqrt_cov.cholesky_decomposition(); //throws MathException if not possible
    inverse_cov.invert_cholesky();
}

double gauss::width(const ParId & p)const{
    size_t i=0;
    for(vector<ParId>::const_iterator v=v_par_ids.begin(); v!=v_par_ids.end(); ++v, ++i){
        if(*v == p)break;
    }
    const pair<double, double> & range = ranges[i];
    double min_result = range.second - range.first;
    if(min_result = 0.0) return 0.0;

    // in the sampling procedure, sqrt_cov * vector is returned. Now, the
    // width of the marginal distribution of component i of the result vector is given by
    //   max_v   (sqrt_cov * v)[i]
    // where the maximum is taken over all vectors of unit length, v.
    // This maximum is the same as:
    double result = 0;
    for(size_t j=0; j<=i; j++){
        result = std::max(result, fabs(sqrt_cov(i,j)));
    }
    return result;
}

const std::pair<double, double> & gauss::support(const ParId & p)const{
    size_t i=0;
    for(vector<ParId>::const_iterator v=v_par_ids.begin(); v!=v_par_ids.end(); ++v, ++i){
        if(*v == p)return ranges[i];
    }
    throw InvalidArgumentException("gauss::support(): invalid parameter");
}

void gauss::mode(theta::ParValues & result) const{
    size_t i=0;
    for(vector<ParId>::const_iterator v=v_par_ids.begin(); v!=v_par_ids.end(); ++v, ++i){
        result.set(*v, mu[i]);
    }
}


mult::mult(const Configuration & cfg): Function(cfg){
    size_t n = cfg.setting["parameters"].size();
    if(n==0){
        throw ConfigurationException("mult: 'parameters' empty (or not a list)!");
    }
    SettingWrapper s = cfg.setting["parameters"];
    v_pids.reserve(n);
    for(size_t i=0; i<n; ++i){
        string parname = s[i];
        ParId pid = cfg.vm->getParId(parname);
        par_ids.insert(pid);
        v_pids.push_back(pid);
    }
}

double mult::operator()(const ParValues & v) const{
    double result = 1.0;
    for(vector<ParId>::const_iterator it=v_pids.begin(); it!=v_pids.end(); ++it){
        result *= v.get(*it);
    }
    return result;
}

//product_distribution

void product_distribution::add_distributions(const Configuration & cfg, const theta::SettingWrapper & s, int depth){
    if(depth==0) throw ConfigurationException("product_distribution: nesting too deep while trying to resolve distributions");
    if(s.size()==0) throw ConfigurationException("product_distribution: setting list \"distributions\" is empty");
    for(size_t i=0; i<s.size(); ++i){
        SettingWrapper dist_setting = s[i];
        string dist_setting_type = dist_setting["type"]; 
        if(dist_setting_type=="product_distribution"){
            add_distributions(cfg, dist_setting["distributions"], depth-1);
        }
        else{
            distributions.push_back(PluginManager<Distribution>::build(Configuration(cfg, dist_setting)));
            ParIds new_pids = distributions.back().getParameters();
            par_ids.insert(new_pids.begin(), new_pids.end());
            for(ParIds::const_iterator it=new_pids.begin(); it!=new_pids.end(); ++it){
                parid_to_index[*it] = distributions.size()-1;
            }
        }
    }
}

product_distribution::product_distribution(const Configuration & cfg){
    add_distributions(cfg, cfg.setting["distributions"], 10);
}

void product_distribution::sample(ParValues & result, Random & rnd) const{
    const boost::ptr_vector<Distribution>::const_iterator end=distributions.end();
    for(boost::ptr_vector<Distribution>::const_iterator it=distributions.begin(); it!=end; ++it){
        it->sample(result, rnd);
    }
}

void product_distribution::mode(ParValues & result) const{
    const boost::ptr_vector<Distribution>::const_iterator end=distributions.end();
    for(boost::ptr_vector<Distribution>::const_iterator it=distributions.begin(); it!=end; ++it){
        it->mode(result);
    }
}

double product_distribution::evalNL(const ParValues & values) const{
    double result = 0.0;
    const boost::ptr_vector<Distribution>::const_iterator end=distributions.end();
    for(boost::ptr_vector<Distribution>::const_iterator it=distributions.begin(); it!=end; ++it){
        result += it->evalNL(values);
    }
    return result;
}

double product_distribution::evalNL_withDerivatives(const ParValues & values, ParValues & derivatives) const {
    double result = 0.0;
    const boost::ptr_vector<Distribution>::const_iterator end = distributions.end();
    for (boost::ptr_vector<Distribution>::const_iterator it = distributions.begin(); it != end; ++it) {
        result += it->evalNL_withDerivatives(values, derivatives);
    }
    return result;
}


const std::pair<double, double> & product_distribution::support(const ParId & p) const{
    map<ParId, size_t>::const_iterator it = parid_to_index.find(p);
    if(it==parid_to_index.end()) throw InvalidArgumentException("product_distribution::support: invalid ParId");
    return distributions[it->second].support(p);
}

double product_distribution::width(const ParId & p) const{
    map<ParId, size_t>::const_iterator it = parid_to_index.find(p);
    if(it==parid_to_index.end()) throw InvalidArgumentException("product_distribution::width: invalid ParId");
    return distributions[it->second].width(p);
}

void model_source::define_table(){
    for(size_t i=0; i< parameter_names.size(); ++i){
        parameter_columns.push_back(table->add_column(*this, parameter_names[i], EventTable::typeDouble));
    }
    if(save_nll!=nosave){
        c_nll = table->add_column(*this, "nll", EventTable::typeDouble);
    }
}

model_source::model_source(const theta::plugin::Configuration & cfg): save_nll(nosave), DataSource(cfg){
    model = ModelFactory::buildModel(Configuration(cfg, cfg.setting["model"]));
    obs_ids = model->getObservables();
    par_ids = model->getParameters();
    for(ParIds::const_iterator p_it=par_ids.begin(); p_it!=par_ids.end(); ++p_it){
        parameter_names.push_back(cfg.vm->getName(*p_it));
    }
    if(cfg.setting.exists("override-parameter-distribution")){
        override_parameter_distribution = PluginManager<Distribution>::build(Configuration(cfg, cfg.setting["override-parameter-distribution"]));
    }
    if(cfg.setting.exists("save-nll")){
        string s_save_nll = cfg.setting["save-nll"];
        if(s_save_nll == "distribution-from-model"){
            save_nll = distribution_from_model;
        }
        else if (s_save_nll == "distribution-from-override"){
            save_nll = distribution_from_override;
            if(override_parameter_distribution.get()==0){
                throw ConfigurationException("model_source: save-nll set to \"distribution from override\", but no \"override-parameter-distribution\" specified!");
            }
        }
        else if(s_save_nll == ""){
            throw ConfigurationException("model_source: invalid setting save-nll specified (allowed "
                     "values are \"\", \"distribution-from-model\" and \"distribution-from-override\")");
        }
    }
}

void model_source::fill(Data & dat, Run & run){
    Random & rnd = run.get_random();
    ParValues values;
    if(override_parameter_distribution.get()){
        override_parameter_distribution->sample(values, rnd);
    }
    else{
        model->get_parameter_distribution().sample(values, rnd);
    }
    model->samplePseudoData(dat, rnd, values);
    size_t i=0;
    for(ParIds::const_iterator p_it=par_ids.begin(); p_it!=par_ids.end(); ++p_it, ++i){
        table->set_column(parameter_columns[i], values.get(*p_it));
    }
    
    if(save_nll==nosave) return;
    NLLikelihood nll = model->getNLLikelihood(dat);
    if(save_nll==distribution_from_override) nll.set_override_distribution(override_parameter_distribution.get());
    table->set_column(c_nll, nll(values));
}


REGISTER_PLUGIN(gauss)
REGISTER_PLUGIN(log_normal)
REGISTER_PLUGIN(flat_distribution)
REGISTER_PLUGIN(delta_distribution)

REGISTER_PLUGIN(fixed_poly)
REGISTER_PLUGIN(fixed_gauss)
REGISTER_PLUGIN(mult)
REGISTER_PLUGIN(product_distribution)
REGISTER_PLUGIN(model_source)
