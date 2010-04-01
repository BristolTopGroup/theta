#include "plugins/mcmc_quantiles.hpp"
#include "plugins/mcmc.hpp"
#include "interface/plugin.hpp"
#include "interface/run.hpp"
#include "interface/minimizer.hpp"
#include "interface/histogram.hpp"

#include <sstream>

using namespace theta;
using namespace std;
using namespace libconfig;

void mcmc_quantiles::define_table(){
    for(size_t i=0; i<quantiles.size(); ++i){
        stringstream ss;
        ss << "quant" << setw(5) << setfill('0') << static_cast<int>(quantiles[i] * 10000 + 0.5);
        columns.push_back(table->add_column(*this, ss.str(), EventTable::typeDouble));
    }
}

//the result class for the metropolisHastings routine.
class MCMCPosteriorQuantilesResult{
    public:
        //ipar_ is the parameter of interest
        MCMCPosteriorQuantilesResult(size_t npar_, size_t ipar_, size_t n_iterations_): npar(npar_), ipar(ipar_), n_iterations(n_iterations_){
            par_values.reserve(n_iterations);
        }
        
        size_t getnpar() const{
            return npar;
        }
        
        //just save the parameter value we are interested in
        void fill(const double * x, double, size_t n_){
            for(size_t i=0; i<n_; ++i){
               par_values.push_back(x[ipar]);
            }
        }

        void end(){}
        
        //return the quantile q
        double get_quantile(double q){
            if(par_values.size()!=n_iterations){
                throw InvalidArgumentException("MCMCPosteriorQuantilesResult: called get_quantile before chain has finished!");
            }
            int index = static_cast<int>(q * n_iterations);
            if(index >= static_cast<int>(n_iterations)) index = n_iterations-1;
            std::nth_element(par_values.begin(), par_values.begin() + index, par_values.end());
            return par_values[index];
        }
        
    private:
        size_t npar;
        size_t ipar;
        size_t n_iterations;
        vector<double> par_values;
};

void mcmc_quantiles::produce(Run & run, const Data & data, const Model & model) {
    if(!init){
        try{
            if(init_failed){
                throw Exception("init failed earlier, not attempting initialization again");
            }
            Histogram pred;
            ParValues default_parameters = vm->get_defaults();
            
            ObsIds observables = model.getObservables();
            Data d;
            for(ObsIds::const_iterator it=observables.begin(); it!=observables.end(); ++it){
                model.get_prediction(pred, default_parameters, *it);
                d.addData(*it, pred);
            }
            NLLikelihood nll = model.getNLLikelihood(d);
            sqrt_cov = get_sqrt_cov(run.get_random(), nll, ParValues(), vm, startvalues);
            
            ParIds nll_pars = nll.getParameters();
            ipar=0;
            for(ParIds::const_iterator it=nll_pars.begin(); it!=nll_pars.end(); ++it, ++ipar){
                if(*it == par_id) break;
            }
            //now ipar has the correct value ...
            
            init = true;
        }catch(Exception & ex){
            init_failed = true;
            ex.message = "initialization failed with: " + ex.message;
            throw;
        }
    }
    
    NLLikelihood nll = model.getNLLikelihood(data);
    MCMCPosteriorQuantilesResult result(nll.getnpar(), ipar, iterations);
    metropolisHastings(nll, result, run.get_random(), startvalues, sqrt_cov, iterations, burn_in);
    
    for(size_t i=0; i<quantiles.size(); ++i){
        table->set_column(columns[i], result.get_quantile(quantiles[i]));
    }
}

mcmc_quantiles::mcmc_quantiles(const theta::plugin::Configuration & cfg): Producer(cfg), init(false), init_failed(false){
    vm = cfg.vm;
    SettingWrapper s = cfg.setting;
    string parameter = s["parameter"];
    par_id = vm->getParId(parameter);
    size_t n = s["quantiles"].size();
    if(n==0){
        throw ConfigurationException("mcmc_quantiles: list of requested quantiles is empty");
    }
    quantiles.reserve(n);
    for(size_t i=0; i<n; ++i){
        quantiles.push_back(s["quantiles"][i]);
        if(quantiles[i]<=0.0 || quantiles[i]>=1.0){
            throw ConfigurationException("mcmc_quantiles: quantiles out of range (must be strictly between 0 and 1)");
        }
    }
    iterations = s["iterations"];
    if(s.exists("burn-in")){
        burn_in = s["burn-in"];
    }
    else{
        burn_in = iterations / 10;
    }
}

REGISTER_PLUGIN(mcmc_quantiles)
