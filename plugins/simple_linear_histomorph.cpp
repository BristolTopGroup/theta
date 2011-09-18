#include "plugins/simple_linear_histomorph.hpp"

using namespace std;
using namespace theta;
using namespace theta::plugin;
using namespace libconfig;

const Histogram1D & simple_linear_histomorph::operator()(const ParValues & values) const {
    h = h0;
    const size_t n_sys = hplus_diff.size();
    for (size_t isys = 0; isys < n_sys; isys++) {
        const double delta = values.get(vid[isys]);
        if(delta==0.0) continue;
        const Histogram1D & t_sys = delta > 0 ? hplus_diff[isys] : hminus_diff[isys];
        if (t_sys.get_nbins() == 0)continue;
        h.add_with_coeff(fabs(delta), t_sys);
    }
    for(size_t i=0; i<h.get_nbins(); ++i){
       h.set(i, max(h.get(i), 0.0));
    }
    return h;
}

std::auto_ptr<theta::HistogramFunction> simple_linear_histomorph::clone() const{
    return std::auto_ptr<theta::HistogramFunction>(new simple_linear_histomorph(*this));
}

simple_linear_histomorph::simple_linear_histomorph(const Configuration & cfg){
    SettingWrapper psetting = cfg.setting["parameters"];
    //build nominal histogram:
    h0 = getConstantHistogram(cfg, cfg.setting["nominal-histogram"]);
    size_t n = psetting.size();
    boost::shared_ptr<VarIdManager> vm = cfg.pm->get<VarIdManager>();
    for(size_t i=0; i<n; i++){
        string par_name = psetting[i];
        ParId pid = vm->getParId(par_name);
        par_ids.insert(pid);
        vid.push_back(pid);
        string setting_name;
        //plus:
        setting_name = par_name + "-plus-histogram";
        if(cfg.setting.exists(setting_name)){
           hplus_diff.push_back(getConstantHistogram(cfg, cfg.setting[setting_name]));
           hplus_diff.back().check_compatibility(h0);
           hplus_diff.back().add_with_coeff(-1.0, h0);
        }
        else{
           hplus_diff.push_back(Histogram1D());
        }
        //minus:
        setting_name = par_name + "-minus-histogram";
        if(cfg.setting.exists(setting_name)){
           hminus_diff.push_back(getConstantHistogram(cfg, cfg.setting[setting_name]));
           hminus_diff.back().check_compatibility(h0);
           hminus_diff.back().add_with_coeff(-1.0, h0);
        }
        else{
           hminus_diff.push_back(Histogram1D());
        }
    }
    assert(hplus_diff.size()==hminus_diff.size());
    assert(vid.size()==hminus_diff.size());
    assert(vid.size()==n);
}

Histogram1D simple_linear_histomorph::getConstantHistogram(const Configuration & cfg, SettingWrapper s){
    std::auto_ptr<HistogramFunction> hf = PluginManager<HistogramFunction>::instance().build(Configuration(cfg, s));
    if(hf->getParameters().size()!=0){
        stringstream ss;
        ss << "Histogram defined in path " << s.getPath() << " is not constant (but has to be).";
        throw InvalidArgumentException(ss.str());
    }
    return (*hf)(ParValues());
}

REGISTER_PLUGIN(simple_linear_histomorph)
