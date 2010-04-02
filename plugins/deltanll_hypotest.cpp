#include "plugins/deltanll_hypotest.hpp"
#include "interface/plugin.hpp"
#include "interface/run.hpp"
#include "interface/minimizer.hpp"
#include "interface/histogram.hpp"
#include "interface/distribution.hpp"

#include <sstream>

using namespace theta;
using namespace std;
using namespace libconfig;

void deltanll_hypotest::define_table(){
    c_nll_b = table->add_column(*this, "nll_b", EventTable::typeDouble);
    c_nll_sb = table->add_column(*this, "nll_sb", EventTable::typeDouble);
}

void deltanll_hypotest::produce(theta::Run & run, const theta::Data & data, const theta::Model & model){
    NLLikelihood nll = get_nllikelihood(data, model);
    nll.set_override_distribution(s_plus_b.get());
    MinimizationResult minres = minimizer->minimize(nll, s_plus_b_mode, s_plus_b_width, s_plus_b_support);
    double nll_sb = minres.fval;
    
    nll.set_override_distribution(b_only.get());
    minres = minimizer->minimize(nll, b_only_mode, b_only_width, b_only_support);
    double nll_b = minres.fval;
    
    table->set_column(c_nll_sb, nll_sb);
    table->set_column(c_nll_b, nll_b);
}


deltanll_hypotest::deltanll_hypotest(const theta::plugin::Configuration & cfg):
        Producer(cfg){
    SettingWrapper s = cfg.setting;
    if(s.exists("override-parameter-distribution")) throw ConfigurationException("deltanll_hypotest: override-parameter-distribution is not allowed here");
    minimizer = theta::plugin::PluginManager<Minimizer>::build(theta::plugin::Configuration(cfg, s["minimizer"]));
    
    s_plus_b = theta::plugin::PluginManager<Distribution>::build(theta::plugin::Configuration(cfg, s["signal-plus-background-distribution"]));
    b_only = theta::plugin::PluginManager<Distribution>::build(theta::plugin::Configuration(cfg, s["background-only-distribution"]));
    
    DistributionUtils::fillModeWidthSupport(s_plus_b_mode, s_plus_b_width, s_plus_b_support, *s_plus_b);
    DistributionUtils::fillModeWidthSupport(b_only_mode, b_only_width, b_only_support, *b_only);
}

REGISTER_PLUGIN(deltanll_hypotest)
