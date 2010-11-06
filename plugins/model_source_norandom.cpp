#include "plugins/model_source_norandom.hpp"
#include "interface/model.hpp"

using namespace theta;
using namespace theta::plugin;
using namespace std;

model_source_norandom::model_source_norandom(const theta::plugin::Configuration & cfg): DataSource(cfg){
    model = PluginManager<Model>::build(Configuration(cfg, cfg.setting["model"]));
    SettingWrapper pv_s = cfg.setting["parameter-values"];
    size_t n = pv_s.size();
    for(size_t i=0; i<n; ++i){
        string par_name = pv_s[i][0];
        values.set(cfg.vm->getParId(par_name), pv_s[i][1]);
    }
    ObsIds observables = model->getObservables();
    model->get_prediction(data, values);
}


void model_source_norandom::fill(theta::Data & dat, theta::Run &){
    dat = data;
}

REGISTER_PLUGIN(model_source_norandom)
