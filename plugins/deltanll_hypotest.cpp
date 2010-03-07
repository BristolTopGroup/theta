#include "plugins/deltanll_hypotest.hpp"
#include "interface/plugin.hpp"
#include "interface/run.hpp"
#include "interface/minimizer.hpp"
#include "interface/histogram.hpp"

#include <sstream>

using namespace theta;
using namespace std;
using namespace libconfig;

void deltanll_hypotest::define_table(){
    c_nll_b = table->add_column(*this, "nll_b", ProducerTable::typeDouble);
    c_nll_sb = table->add_column(*this, "nll_sb", ProducerTable::typeDouble);
}

void deltanll_hypotest::produce(Run & run, const Data & data, const Model & model) {
    NLLikelihood nll = model.getNLLikelihood(data);
    //see above for this type of noop_deleter construction
    double nll_sb, nll_b;
    nll_sb = NAN;
    nll_b = NAN;
    //a. calculate s plus b:
    //apply s_plus_b constraints and take back any others:
    for(ParIds::const_iterator it = par_ids_constraints.begin(); it!=par_ids_constraints.end(); ++it){
        if(s_plus_b.contains(*it)){
            minimizer->override_range(*it, s_plus_b.get(*it), s_plus_b.get(*it));
        }else{
            minimizer->reset_override(*it);
        }
    }
    MinimizationResult minres = minimizer->minimize(nll);
    nll_sb = minres.fval;
    
    //b. calculate b only:
    //apply b contraints:
    for(ParIds::const_iterator it = par_ids_constraints.begin(); it!=par_ids_constraints.end(); ++it){
        if(b_only.contains(*it))
            minimizer->override_range(*it, b_only.get(*it), b_only.get(*it));
        else
            minimizer->reset_override(*it);
    }
    minres = minimizer->minimize(nll);
    nll_b = minres.fval;
    table->set_column(c_nll_sb, nll_sb);
    table->set_column(c_nll_b, nll_b);
}


deltanll_hypotest::deltanll_hypotest(const theta::plugin::Configuration & cfg): Producer(cfg){
    SettingWrapper s = cfg.setting;
    minimizer = theta::plugin::PluginManager<Minimizer>::build(theta::plugin::Configuration(cfg, s["minimizer"]));
    size_t sb_i = s["signal-plus-background"].size();
    for (size_t i = 0; i < sb_i; ++i) {
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
    par_ids_constraints = s_plus_b.getAllParIds();
    ParIds par_ids_b = b_only.getAllParIds();
    for(ParIds::const_iterator it=par_ids_b.begin(); it!=par_ids_b.end(); ++it){
        par_ids_constraints.insert(*it);
    }    
}

REGISTER_PLUGIN(deltanll_hypotest)
