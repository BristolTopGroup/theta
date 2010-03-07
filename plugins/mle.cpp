#include "plugins/mle.hpp"
#include "interface/plugin.hpp"
#include "interface/run.hpp"
#include "interface/minimizer.hpp"
#include "interface/histogram.hpp"

#include <sstream>

using namespace theta;
using namespace std;
using namespace libconfig;
using namespace theta::plugin;

void mle::define_table(){
    c_nll = table->add_column(*this, "nll", ProducerTable::typeDouble);
    for(size_t i=0; i<save_ids.size(); ++i){
        parameter_columns.push_back(table->add_column(*this, vm->getName(save_ids[i]), ProducerTable::typeDouble));
        error_columns.push_back(table->add_column(*this, vm->getName(save_ids[i]) + "_error", ProducerTable::typeDouble));
    }
}

void mle::produce(Run & run, const Data & data, const Model & model) {
    NLLikelihood nll = model.getNLLikelihood(data);
    MinimizationResult minres = minimizer->minimize(nll);
    table->set_column(c_nll, minres.fval);
    for(size_t i=0; i<save_ids.size(); ++i){
        table->set_column(parameter_columns[i], minres.values.get(save_ids[i]));
        table->set_column(error_columns[i], 0.5 * (minres.errors_plus.get(save_ids[i]) + minres.errors_minus.get(save_ids[i])) );
    }
}

mle::mle(const theta::plugin::Configuration & cfg): Producer(cfg), vm(cfg.vm){
    SettingWrapper s = cfg.setting;
    minimizer = PluginManager<Minimizer>::build(Configuration(cfg, s["minimizer"]));
    size_t n_parameters = s["parameters"].size();
    for (size_t i = 0; i < n_parameters; i++) {
        string par_name = s["parameters"][i];
        save_ids.push_back(cfg.vm->getParId(par_name));
    }
    //table.init(*cfg.vm, save_ids);
}

REGISTER_PLUGIN(mle)
