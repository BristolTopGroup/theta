#include "plugins/mle.hpp"
#include "interface/plugin.hpp"
#include "interface/run.hpp"
#include "interface/minimizer.hpp"
#include "interface/histogram.hpp"
#include "interface/distribution.hpp"

#include <sstream>

using namespace theta;
using namespace std;
using namespace libconfig;
using namespace theta::plugin;

void mle::define_table(){
    c_nll = table->add_column(get_name(), "nll", Table::typeDouble);
    for(size_t i=0; i<save_ids.size(); ++i){
        parameter_columns.push_back(table->add_column(get_name(), parameter_names[i], Table::typeDouble));
        error_columns.push_back(table->add_column(get_name(), parameter_names[i] + "_error", Table::typeDouble));
    }
}

void mle::produce(theta::Run & run, const theta::Data & data, const theta::Model & model) {
    NLLikelihood nll = get_nllikelihood(data, model);
    if(not start_step_ranges_init){
        const Distribution & d = nll.get_parameter_distribution();
        DistributionUtils::fillModeWidthSupport(start, step, ranges, d);
        start_step_ranges_init = true;
    }
    MinimizationResult minres = minimizer->minimize(nll, start, step, ranges);
    table->set_column(*c_nll, minres.fval);
    for(size_t i=0; i<save_ids.size(); ++i){
        table->set_column(parameter_columns[i], minres.values.get(save_ids[i]));
        table->set_column(error_columns[i], 0.5 * (minres.errors_plus.get(save_ids[i]) + minres.errors_minus.get(save_ids[i])) );
    }
}

mle::mle(const theta::plugin::Configuration & cfg): Producer(cfg), start_step_ranges_init(false){
    SettingWrapper s = cfg.setting;
    minimizer = PluginManager<Minimizer>::build(Configuration(cfg, s["minimizer"]));
    size_t n_parameters = s["parameters"].size();
    for (size_t i = 0; i < n_parameters; i++) {
        string par_name = s["parameters"][i];
        save_ids.push_back(cfg.vm->getParId(par_name));
        parameter_names.push_back(par_name);
    }
}

REGISTER_PLUGIN(mle)
