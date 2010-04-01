#include "core.hpp"
#include "plugins/interpolating-histogram.hpp"
#include "interface/random.hpp"

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

void log_normal::mpv(theta::ParValues & result) const{
    result.set(*(par_ids.begin()), exp(mu - sigma*sigma));
}

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
            ranges[0].first = cfg.setting["range"][0];
            ranges[0].second = cfg.setting["range"][1];
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
    for(size_t i=0; i<n; ++i){
        string parname = s[i];
        par_ids.insert(cfg.vm->getParId(parname));
    }
}

double mult::operator()(const ParValues & v) const{
    double result = 1.0;
    for(ParIds::const_iterator it=par_ids.begin(); it!=par_ids.end(); it++){
        result *= v.get(*it);
    }
    return result;
}

product_distribution::product_distribution(const Configuration & cfg){

}

void product_distribution::sample(ParValues & result, Random & rnd) const{
    const boost::ptr_vector<Distribution>::const_iterator end=distributions.end();
    for(boost::ptr_vector<Distribution>::const_iterator it=distributions.begin(); it!=end; ++it){
        it->sample(result, rnd);
    }
}

void product_distribution::mpv(ParValue & result) const{
    const boost::ptr_vector<Distribution>::const_iterator end=distributions.end();
    for(boost::ptr_vector<Distribution>::const_iterator it=distributions.begin(); it!=end; ++it){
        it->mpv(result);
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
    if(it==parid_to_index.end()) throw IllegalArgumentException("product_distribution::support: invalid ParId");
    return distributions[it->second].support(p);
}

double product_distribution::width(const ParId & p) const{
    map<ParId, size_t>::const_iterator it = parid_to_index.find(p);
    if(it==parid_to_index.end()) throw IllegalArgumentException("product_distribution::width: invalid ParId");
    return distributions[it->second].width(p);
}


REGISTER_PLUGIN(gauss)
REGISTER_PLUGIN(log_normal)
REGISTER_PLUGIN(fixed_poly)
REGISTER_PLUGIN(fixed_gauss)
REGISTER_PLUGIN(mult)
REGISTER_PLUGIN(product_distribution)
