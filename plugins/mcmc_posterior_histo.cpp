#include "plugins/mcmc_posterior_histo.hpp"
#include "plugins/mcmc.hpp"
#include "interface/plugin.hpp"
#include "interface/run.hpp"
#include "interface/histogram.hpp"
#include "interface/distribution.hpp"

#include <sstream>

using namespace theta;
using namespace std;
using namespace libconfig;

namespace{

//the result class for the metropolisHastings routine, saving the histograms
class MCMCPosteriorHistoResult{
    public:
        //ipar_ is the parameter of interest
        MCMCPosteriorHistoResult(const vector<size_t> & ipars_, size_t npar_, const vector<size_t> & nbins,
                                 const vector<double> & lower, const vector<double> & upper):
                           npar(npar_), ipars(ipars_)
        {   
            assert(ipars_.size() == nbins.size());
            assert(ipars_.size() == lower.size());
            assert(ipars_.size() == upper.size());
            histos.resize(ipars_.size());
            for(size_t i=0; i<ipars_.size(); ++i){
                histos[i].reset(nbins[i], lower[i], upper[i]);
            }
        }
        
        size_t getnpar() const{
            return npar;
        }
        
        //fill the parameters we are interested in into their histograms ...
        void fill(const double * x, double, size_t n_){
            for(size_t i=0; i<ipars.size(); ++i){
                histos[i].fill(x[ipars[i]], n_);
            }
        }

        void end(){}
        
        const theta::Histogram & get_histo(size_t i){
            return histos[i];
        }
        
    private:
        size_t npar;
        vector<size_t> ipars;
        vector<theta::Histogram> histos;
};


struct nll_smoothed{
   nll_smoothed(const NLLikelihood & nll_, size_t ipar_, size_t nbins_, double xmin_, double xmax_): nll(nll_), ipar(ipar_), nbins(nbins_),
      xmin(xmin_), xmax(xmax_), binwidth((xmax - xmin)/nbins), histo(nbins, xmin, xmax){
          npar = nll.getnpar();
          assert(ipar < npar);
      }
   
   double operator()(const double * values) const {
       vector<double> myvalues(npar);
       copy(values, values + npar, myvalues.begin());
       double nll_min = std::numeric_limits<double>::infinity();
       for(size_t i=0; i<nbins; ++i){
           double x = xmin + (i + 0.5) * binwidth;
           myvalues[ipar] = x;
           double nll_x = nll(&myvalues[0]);
           histo.set(i+1, nll_x);
           nll_min = min(nll_min, nll_x);
       }
       double result = 0.0;
       for(size_t i=0; i<nbins; ++i){
           result += exp(nll_min - histo.get(i+1));
       }
       return nll_min - log(result);
   }
   
   size_t getnpar() const{
       return npar;
   }
   
   const NLLikelihood & nll;
   size_t ipar, nbins, npar;
   double xmin, xmax, binwidth;
   
   mutable Histogram histo;
};

// the result class for the smoothed version:
class MCMCPosteriorHistoResultSmoothed{
public:
    MCMCPosteriorHistoResultSmoothed(size_t nbins, double xmin, double xmax, const nll_smoothed & nll_): npar(npar), histo(nbins, xmin, xmax),histo_tmp(nbins, xmin, xmax),
      nll(nll_){
       npar = nll.getnpar();
    }
    
    void fill(const double * x, double nll_value, size_t n){
        nll(x);
        for(size_t i=1; i<=histo.get_nbins(); ++i){
            double nll_diff = nll.histo.get(i) - nll_value;
            histo_tmp.set(i, exp(-nll_diff));
        }
        histo.add_with_coeff(n / histo_tmp.get_sum_of_bincontents(), histo_tmp);
    }
    
    size_t getnpar() const{
        return npar;
    }
    
    void end(){}
    
    const Histogram & get_histo() const{
        return histo;
    }
private:
    size_t npar;
    Histogram histo, histo_tmp;
    const nll_smoothed & nll;
};


}



void mcmc_posterior_histo::produce(Run & run, const Data & data, const Model & model) {
    if(!init){
        try{
            //get the covariance for average data:
            sqrt_cov = get_sqrt_cov2(*rnd_gen, model, startvalues, override_parameter_distribution, vm);
            
            //find ipars:
            ParIds nll_pars = model.getParameters();
            ipars.resize(parameters.size());
            for(size_t i=0; i<parameters.size(); ++i){
                for(ParIds::const_iterator it=nll_pars.begin(); it!=nll_pars.end(); ++it, ++ipars[i]){
                    if(*it == parameters[i]) break;
                }
            }
            
            init = true;
        }
        catch(NotFoundException & ex){
            ex.message = "initialization failed: " + ex.message + " (did you specify widths for all unbound parameters?)";
            throw FatalException(ex);
        }
        catch(Exception & ex){
            ex.message = "initialization failed: " + ex.message;
            throw FatalException(ex);
        }
    }
    
    std::auto_ptr<NLLikelihood> nll = get_nllikelihood(data, model);
    if(!smooth){
        MCMCPosteriorHistoResult result(ipars, nll->getnpar(), nbins, lower, upper);
        metropolisHastings(*nll, result, *rnd_gen, startvalues, sqrt_cov, iterations, burn_in);
    
        for(size_t i=0; i<parameters.size(); ++i){
            products_sink->set_product(columns[i], result.get_histo(i));
        }
    }
    else{
        nll_smoothed nll_s(*nll, ipars[0], nbins[0], lower[0], upper[0]);
        MCMCPosteriorHistoResultSmoothed result(nbins[0], lower[0], upper[0], nll_s);
        metropolisHastings(nll_s, result, *rnd_gen, startvalues, sqrt_cov, iterations, burn_in);
        products_sink->set_product(columns[0], result.get_histo());
    }
}

mcmc_posterior_histo::mcmc_posterior_histo(const theta::plugin::Configuration & cfg): Producer(cfg), RandomConsumer(cfg, getName()),
        init(false), smooth(false){
    SettingWrapper s = cfg.setting;
    vm = cfg.vm;
    size_t n = s["parameters"].size();
    for(size_t i=0; i<n; ++i){
        string parameter_name = s["parameters"][i];
        parameter_names.push_back(parameter_name);
        parameters.push_back(cfg.vm->getParId(parameter_name));
        nbins.push_back(static_cast<unsigned int>(s["histo_" + parameter_name]["nbins"]));
        lower.push_back(s["histo_" + parameter_name]["range"][0]);
        upper.push_back(s["histo_" + parameter_name]["range"][1]);
    }
    
    iterations = s["iterations"];
    if(s.exists("burn-in")){
        burn_in = s["burn-in"];
    }
    else{
        burn_in = iterations / 10;
    }
    if(s.exists("smooth")){
        smooth = s["smooth"];
        if(smooth && parameters.size()!=1){
            throw ConfigurationException("enabling 'smooth' is only supported if making the histogram in only one parameter!");
        }
    }
    for(size_t i=0; i<parameters.size(); ++i){
        columns.push_back(products_sink->declare_product(*this, "posterior_" + parameter_names[i], theta::typeHisto));
    }
}

REGISTER_PLUGIN(mcmc_posterior_histo)

