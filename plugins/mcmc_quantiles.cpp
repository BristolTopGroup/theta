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

void mcmc_quantiles_table::create_table() {
    stringstream ss;
    ss << "CREATE TABLE '" << name << "' (runid INTEGER(4), eventid INTEGER(4)";
    exec(ss.str());
    ss.str("");
    ss << "INSERT INTO '" << name << "' VALUES(?,?,";
    insert_statement = prepare(ss.str());
}

void mcmc_quantiles_table::append(const Run & run, const vector<double> & quantiles) {
    sqlite3_bind_int(insert_statement, 1, run.get_runid());
    sqlite3_bind_int(insert_statement, 2, run.get_eventid());
    int next_col = 3;
    for(size_t i=0; i<quantiles.size(); ++i){
        sqlite3_bind_double(insert_statement, next_col, quantiles[i]);
        ++next_col;
    }
    int res = sqlite3_step(insert_statement);
    sqlite3_reset(insert_statement);
    if (res != 101) {
        error(__FUNCTION__);//throws exception
    }
}

//the result class for the metropolisHastings routine.
/*class MCMCPosteriorRatioResult{
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
};*/

void mcmc_quantiles::produce(Run & run, const Data & data, const Model & model) {
    if(!table) table.connect(run.get_database());
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
            
            init = true;
        }catch(Exception & ex){
            init_failed = true;
            ex.message = "initialization failed with: " + ex.message;
            throw;
        }
    }
    
    NLLikelihood nll = model.getNLLikelihood(data);
    //TODO: save result, return quantiles
    vector<double> quants;
    //MCMCPosteriorRatioResult res_sb(nll.getnpar());
    //metropolisHastings(nll, res_sb, run.get_random(), startvalues_sb, sqrt_cov_sb, iterations, burn_in);
    
    table.append(run, quants);
}

std::string mcmc_quantiles::get_information() const{
    stringstream ss;
    for(vector<double>::const_iterator it=quantiles.begin(); it!=quantiles.end(); ++it){
        ss << *it << " ";
    }
    return ss.str();
}

mcmc_quantiles::mcmc_quantiles(const theta::plugin::Configuration & cfg): Producer(cfg), init(false), init_failed(false), table(get_name()){
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
    }
    iterations = s["iterations"];
    if(s.exists("burn-in")){
        burn_in = s["burn-in"];
    }
    else{
        burn_in = iterations / 10;
    }
}

//REGISTER_PLUGIN(mcmc_quantiles)
