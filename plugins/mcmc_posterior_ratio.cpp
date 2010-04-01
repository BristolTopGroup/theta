#include "plugins/mcmc_posterior_ratio.hpp"
#include "plugins/mcmc.hpp"
#include "interface/plugin.hpp"
#include "interface/run.hpp"
#include "interface/minimizer.hpp"
#include "interface/histogram.hpp"

#include <sstream>

using namespace theta;
using namespace std;
using namespace libconfig;

//the result class for the metropolisHastings routine.
class MCMCPosteriorRatioResult{
    public:
        MCMCPosteriorRatioResult(size_t npar_): npar(npar_), min_nll_value(numeric_limits<double>::infinity()), n_total(0){}
        
        size_t getnpar() const{
            return npar;
        }
        
        void fill(const double *, double nll, size_t n_){
            nll_values.push_back(nll);
            if(nll < min_nll_value) min_nll_value = nll;
            n.push_back(n_);
            n_total += n_;
        }
        
        void end(){}
        
        //return the negative logarithm of the average posterior
        double get_nl_average_posterior(){
            double posterior_sum = 0.0;
            //instead of calculating
            // - log (   1/N  * sum_{i=1}^N exp (-nll_i)    )
            // calculate
            // - log (  exp(-min_nll) * 1/N * sum_{i=1}^N  exp(-nll_i + min_nll)   ) = min_nll - log (  1/N * sum_{i=1}^N  exp (-nll_i + min_nll)   )
            // which is the same , but numerically much better
            for(size_t i=0; i<n.size(); ++i){
                posterior_sum += n[i] * exp(min_nll_value - nll_values[i]);
            }
            return min_nll_value - log(posterior_sum / n_total);
        }
        
        double get_jump_rate(){
            return (1.0 * n.size()) / n_total;
        }
    private:
        size_t npar;
        vector<double> nll_values;
        double min_nll_value;
        vector<size_t> n;
        size_t n_total;
};

void mcmc_posterior_ratio::produce(Run & run, const Data & data, const Model & model) {
    if(!init){
        try{
            if(init_failed){
                throw Exception("init failed earlier, not attempting initialization again");
            }
            Histogram pred;
            ParValues default_parameters = vm->get_defaults();
            ObsIds observables = model.getObservables();
        
            //1. for signal plus background
            ParValues parameters = default_parameters;
            parameters.set(s_plus_b);
            Data d;
            for(ObsIds::const_iterator it=observables.begin(); it!=observables.end(); ++it){
                model.get_prediction(pred, parameters, *it);
                d.addData(*it, pred);
            }
            NLLikelihood nll_sb = model.getNLLikelihood(d);
            sqrt_cov_sb = get_sqrt_cov(run.get_random(), nll_sb, s_plus_b, vm, startvalues_sb);
        
            //2. for background only
            parameters = default_parameters;
            parameters.set(b_only);
            Data d2;
            for(ObsIds::const_iterator it=observables.begin(); it!=observables.end(); ++it){
                model.get_prediction(pred, parameters, *it);
                d2.addData(*it, pred);
            }
            NLLikelihood nll_b = model.getNLLikelihood(d2);
            sqrt_cov_b = get_sqrt_cov(run.get_random(), nll_b, b_only, vm, startvalues_b);
            
            init = true;
        }catch(Exception & ex){
            init_failed = true;
            ex.message = "initialization failed with: " + ex.message;
            throw;
        }
    }
    
    NLLikelihood nll = model.getNLLikelihood(data);
    double nl_posterior_sb, nl_posterior_b;
    nl_posterior_sb = NAN;
    nl_posterior_b = NAN;
    
    //a. calculate s plus b:
    MCMCPosteriorRatioResult res_sb(nll.getnpar());
    metropolisHastings(nll, res_sb, run.get_random(), startvalues_sb, sqrt_cov_sb, iterations, burn_in);
    nl_posterior_sb = res_sb.get_nl_average_posterior();

    //b. calculate b only:
    MCMCPosteriorRatioResult res_b(nll.getnpar());
    metropolisHastings(nll, res_b, run.get_random(), startvalues_b, sqrt_cov_b, iterations, burn_in);
    nl_posterior_b = res_b.get_nl_average_posterior();

    if(std::isnan(nl_posterior_sb) || std::isnan(nl_posterior_b)){
        throw Exception("average posterior was NAN");
    }
    table->set_column(c_nl_posterior_sb, nl_posterior_sb);
    table->set_column(c_nl_posterior_b, nl_posterior_b);
}


void mcmc_posterior_ratio::define_table(){
    c_nl_posterior_sb = table->add_column(*this, "nl_posterior_sb", EventTable::typeDouble);
    c_nl_posterior_b =  table->add_column(*this, "nl_posterior_b",  EventTable::typeDouble);
}

mcmc_posterior_ratio::mcmc_posterior_ratio(const theta::plugin::Configuration & cfg): Producer(cfg), init(false), init_failed(false){
    vm = cfg.vm;
    SettingWrapper s = cfg.setting;
    size_t sb_i = s["signal-plus-background"].size();
    for (size_t i = 0; i < sb_i; i++) {
        string par_name = s["signal-plus-background"][i].getName();
        double par_value = s["signal-plus-background"][i];
        s_plus_b.set(cfg.vm->getParId(par_name), par_value);
    }
    size_t b_i = s["background-only"].size();
    for (size_t i = 0; i < b_i; i++) {
        string par_name = s["background-only"][i].getName();
        double par_value = s["background-only"][i];
        b_only.set(cfg.vm->getParId(par_name), par_value);
    }
    iterations = s["iterations"];
    if(s.exists("burn-in")){
        burn_in = s["burn-in"];
    }
    else{
        burn_in = iterations / 10;
    }
}

REGISTER_PLUGIN(mcmc_posterior_ratio)
