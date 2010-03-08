#include "plugins/nll_scan.hpp"
#include "plugins/reduced_nll.hpp"
#include "interface/plugin.hpp"
#include "interface/run.hpp"
#include "interface/minimizer.hpp"
#include "interface/histogram.hpp"

#include <sstream>

using namespace theta;
using namespace std;
using namespace libconfig;

void nll_scan::define_table(){
    c_nll = table->add_column(*this, "nll", ProducerTable::typeBlob);
    c_maxl = table->add_column(*this, "maxl", ProducerTable::typeDouble);
}

void nll_scan::produce(Run & run, const Data & data, const Model & model) {
    NLLikelihood nll = model.getNLLikelihood(data);
    MinimizationResult minres = minimizer->minimize(nll);
    table->set_column(c_maxl, minres.values.get(pid));
    ReducedNLL nll_r(nll, pid, minres.values, re_minimize ? minimizer.get() : 0);
    nll_r.set_offset_nll(minres.fval);
    for(unsigned int i=0; i<n_steps; ++i){
        double x = start + i * step;
        result[i] = nll_r(x);
    }
    table->set_column(c_nll, &result[0], result.size() * sizeof(double));
}

nll_scan::nll_scan(const theta::plugin::Configuration & cfg): Producer(cfg), vm(cfg.vm), re_minimize(true){
    SettingWrapper s = cfg.setting;
    minimizer = theta::plugin::PluginManager<Minimizer>::build(theta::plugin::Configuration(cfg, s["minimizer"]));
    string par_name = s["parameter"];
    pid = cfg.vm->getParId(par_name);
    if(s.exists("re-minimize")){
        re_minimize = s["re-minimize"];
    }
    start = s["parameter-values"]["start"];
    stop = s["parameter-values"]["stop"];
    n_steps = s["parameter-values"]["n-steps"];
    if(n_steps<2){
        throw ConfigurationException("nll_scan: n-steps must be >= 2");
    }
    if(start >= stop){
        throw ConfigurationException("nll_scan: start < stop must hold");
    }
    step = (stop - start) / n_steps;
    result.resize(n_steps);
}

REGISTER_PLUGIN(nll_scan)
