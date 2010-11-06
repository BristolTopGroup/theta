#include "plugins/simple_linear_histomorph.hpp"

using namespace std;
using namespace theta;
using namespace theta::plugin;
using namespace libconfig;

const Histogram & simple_linear_histomorph::operator()(const ParValues & values) const {
    h = h0;
    const size_t n_sys = hplus_diff.size();
    for (size_t isys = 0; isys < n_sys; isys++) {
        const double delta = values.get(vid[isys]);
        const Histogram & t_sys = delta > 0 ? hplus_diff[isys] : hminus_diff[isys];
        if (t_sys.get_nbins() == 0)continue;
        h.add_with_coeff(fabs(delta), t_sys);
    }
    for(size_t i=1; i<=h.get_nbins(); ++i){
       h.set(i, max(h.get(i), lower_cutoff));
    }
    h.set(0,0);
    h.set(h.get_nbins() + 1,0);
    return h;
}

theta::ParIds simple_linear_histomorph::getParameters() const {
    theta::ParIds result;
    for (size_t i = 0; i < vid.size(); ++i) {
        result.insert(vid[i]);
    }
    return result;
}

simple_linear_histomorph::simple_linear_histomorph(const Configuration & ctx){
    SettingWrapper psetting = ctx.setting["parameters"];
    //build nominal histogram:
    h0 = getConstantHistogram(ctx, ctx.setting["nominal-histogram"]);
    size_t n = psetting.size();
    for(size_t i=0; i<n; i++){
        string par_name = psetting[i];
        ParId pid = ctx.vm->getParId(par_name);
        vid.push_back(pid);
        string setting_name;
        //plus:
        setting_name = par_name + "-plus-histogram";
        if(ctx.setting.exists(setting_name)){
           hplus_diff.push_back(getConstantHistogram(ctx, ctx.setting[setting_name]));
           hplus_diff.back().add_with_coeff(-1.0, h0);
        }
        else{
           hplus_diff.push_back(h0);
           hplus_diff.back().reset();
        }
        //minus:
        setting_name = par_name + "-minus-histogram";
        if(ctx.setting.exists(setting_name)){
           hminus_diff.push_back(getConstantHistogram(ctx, ctx.setting[setting_name]));
           hminus_diff.back().add_with_coeff(-1.0, h0);
        }
        else{
           hminus_diff.push_back(h0);
           hminus_diff.back().reset();
        }
    }
    assert(hplus_diff.size()==hminus_diff.size());
    assert(vid.size()==hminus_diff.size());
    assert(vid.size()==n);
    h = h0;
    if(ctx.setting.exists("lower_cutoff")){
       lower_cutoff = ctx.setting["lower_cutoff"];
    }
    else{
       lower_cutoff = numeric_limits<double>::infinity();
       for(size_t i=1; i<=h0.get_nbins(); ++i){
          if(h0.get(i) > 0.0){
             lower_cutoff = min(lower_cutoff, 0.01 * h0.get(i));
          }
       }
       assert(!isinf(lower_cutoff));
    }
    cout << "lower cutoff is " << lower_cutoff << endl;
}

Histogram simple_linear_histomorph::getConstantHistogram(const Configuration & cfg, SettingWrapper s){
    std::auto_ptr<HistogramFunction> hf = PluginManager<HistogramFunction>::build(Configuration(cfg, s));
    if(hf->getParameters().size()!=0){
        stringstream ss;
        ss << "Histogram defined in path " << s.getPath() << " is not constant (but has to be).";
        throw InvalidArgumentException(ss.str());
    }
    return (*hf)(ParValues());
}

REGISTER_PLUGIN(simple_linear_histomorph)
