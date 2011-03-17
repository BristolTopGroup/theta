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

//the result class for the metropolisHastings routine.
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
    MCMCPosteriorHistoResult result(ipars, nll->getnpar(), nbins, lower, upper);
    metropolisHastings(*nll, result, *rnd_gen, startvalues, sqrt_cov, iterations, burn_in);
    
    for(size_t i=0; i<parameters.size(); ++i){
        table->set_column(columns[i], result.get_histo(i));
    }
}

mcmc_posterior_histo::mcmc_posterior_histo(const theta::plugin::Configuration & cfg): Producer(cfg), RandomConsumer(cfg, getName()),
        init(false){
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
    for(size_t i=0; i<parameters.size(); ++i){
        columns.push_back(table->add_column(*this, "posterior_" + parameter_names[i], Table::typeHisto));
    }
}

REGISTER_PLUGIN(mcmc_posterior_histo)
