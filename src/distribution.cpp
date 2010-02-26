#include "interface/distribution.hpp"
#include "interface/exception.hpp"
#include "interface/random.hpp"
#include "libconfig/libconfig.h++"

//#include "interface/plugin_so_interface.hpp"
#include "interface/plugin.hpp"

#include <string>
#include <boost/scoped_array.hpp>

using namespace std;
using namespace theta;
using namespace theta::plugin;

LogNormalDistribution::LogNormalDistribution(const ParId & v_id, double mu_, double sigma_) : mu(mu_), sigma(sigma_) {
    par_ids.insert(v_id);
}

double LogNormalDistribution::evalNL(const ParValues & values) const {
    double x = values.get(*par_ids.begin());
    if (x <= 0.0) return numeric_limits<double>::infinity();
    double tmp = (log(x) - mu) / sigma;
    return 0.5 * tmp * tmp + log(x);
}

double LogNormalDistribution::evalNL_withDerivatives(const ParValues & values, ParValues & derivatives) const{
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

void LogNormalDistribution::sample(ParValues & result, AbsRandomProxy & rnd, const VarIdManager & vm) const {
    const ParId & pid = *par_ids.begin();
    const pair<double, double> & range = vm.get_range(pid);
    if(range.first == range.second){
       result.set(pid, range.first);
       return;
    }
    double value;
    int i=0;
    do{
        value = exp(rnd.gauss(sigma) + mu);
        i++;
        //allow many iteratios here ...
        if(i==1000000) throw Exception("LogNormalDistribution::sample: too many iterations necessary to respect bounds!");
    }while(value < range.first || value > range.second);
    result.set(pid, value);
}

void gauss::sample(ParValues & result, AbsRandomProxy & rnd, const VarIdManager & vm) const{
    const size_t n = v_par_ids.size();
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
        for(vector<ParId>::const_iterator v=v_par_ids.begin(); v!=v_par_ids.end(); v++){
            const pair<double, double> & range = vm.get_range(*v);
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
            i++;
        }
        rep++;
        if(rep==1000000) throw Exception("GaussDistribution::sample: too many iterations necessary to respect bounds!");
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

gauss::gauss(Configuration & cfg){
    Matrix cov;
      if(cfg.setting.exists("parameter")){
            mu.resize(1);
            cov.reset(1,1);
            v_par_ids.push_back(cfg.vm->getParId(cfg.setting["parameter"]));
            mu[0] = cfg.setting["mean"];
            double width = cfg.setting["width"];
            cov(0,0) = width*width;
            cfg.rec.markAsUsed(cfg.setting["mean"]);
            cfg.rec.markAsUsed(cfg.setting["parameter"]);
            cfg.rec.markAsUsed(cfg.setting["width"]);
        }
        else{ //multi-dimensional case:
           size_t n = cfg.setting["parameters"].getLength();
           if(n==0){
               stringstream ss;
               ss << "While building gauss distribution defined on line " << cfg.setting.getSourceLine() << ": expected one or more 'parameters'.";
               throw ConfigurationException(ss.str());
           }
           mu.resize(n);
           cov.reset(n,n);
           for(size_t i=0; i<n; i++){
               v_par_ids.push_back(cfg.vm->getParId(cfg.setting["parameters"][i]));
               mu[i] = cfg.setting["mean"][i];
               for(size_t j=0; j<n; j++){
                   cov(i,j) = cfg.setting["covariance"][i][j];
               }
           }
           cfg.rec.markAsUsed(cfg.setting["mean"]);
           cfg.rec.markAsUsed(cfg.setting["parameters"]);
           cfg.rec.markAsUsed(cfg.setting["covariance"]);
      }
      
    const size_t n = v_par_ids.size();
    if (n != mu.size() || n != cov.getRows() || n != cov.getCols())
        throw InvalidArgumentException("GaussDistribution constructor: dimension of parameters do not match.");
    for(vector<ParId>::const_iterator p_it=v_par_ids.begin(); p_it!=v_par_ids.end(); p_it++){
        par_ids.insert(*p_it);
    }
    sqrt_cov = cov;
    inverse_cov = cov;
    sqrt_cov.cholesky_decomposition(); //throws MathException if not possible
    inverse_cov.invert_cholesky();
}

REGISTER_PLUGIN(gauss)

/*class LognormalDistributionFactory: public DistributionFactory{
public:
    std::auto_ptr<Distribution> build(Configuration & ctx)const;
    virtual std::string getTypeName() const{
        return "lognormal";
    }
};

std::auto_ptr<Distribution> LognormalDistributionFactory::build(Configuration & ctx) const {
    double mu = ctx.setting["mu"];
    double sigma = ctx.setting["sigma"];
    string p_name = ctx.setting["parameter"];
    ParId par_id = ctx.vm->getParId(p_name);
    ctx.rec.markAsUsed(ctx.setting["mu"]);
    ctx.rec.markAsUsed(ctx.setting["sigma"]);
    ctx.rec.markAsUsed(ctx.setting["parameter"]);
    return std::auto_ptr<Distribution > (new LogNormalDistribution(par_id, mu, sigma));
}


REGISTER_FACTORY(LognormalDistributionFactory)
*/