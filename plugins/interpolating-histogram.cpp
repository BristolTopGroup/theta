//#include "core.hpp"
//#include "plugin.hpp"

#include "plugins/interpolating-histogram.hpp"

using namespace std;
using namespace theta;
using namespace theta::plugin;
using namespace libconfig;

InterpolatingHistogramFunction::InterpolatingHistogramFunction(const Histogram & h0_, const vector<ParId> & varids,
        const vector<Histogram> & hplus_, const vector<Histogram> & hminus_) :
        h0(h0_), hplus(hplus_), hminus(hminus_), vid(varids), h(h0_){
    const size_t nsys = hplus.size();
    if (nsys != hminus.size() || nsys != varids.size())
        throw InvalidArgumentException("InterpolatingHistogramFunction::InterpolatingHistogramFunction: number of histograms for plus/minus and number of variables mismatch!");
    std::set<ParId> pid_set;
    for(size_t i=0; i<nsys; i++){
        pid_set.insert(varids[i]);
        h0.check_compatibility(hplus[i]);
        h0.check_compatibility(hminus[i]);
        //set overflow and underflow to zero. Those are not included in the likelihood and in order
        // to get the normalization right, we have to subtract those:
        hplus[i].set(0,0);
        hplus[i].set(hplus[i].get_nbins()+1,0);
        hminus[i].set(0,0);
        hminus[i].set(hminus[i].get_nbins()+1,0);
    }
    //to make calculation of derivatives easier, we do not allow the same parameter twice.
    if(pid_set.size()!=nsys){
        throw InvalidArgumentException("InterpolatingHistogramFunction::InterpolatingHistogramFunction: having one parameter parametrizing two interpolations is not supported.");
    }
    h0.set(0,0);
    h0.set(h0.get_nbins()+1,0);
}


const Histogram & InterpolatingHistogramFunction::operator()(const ParValues & values) const {
    h.reset_to_1();
    const size_t n_sys = hplus.size();
    for (size_t isys = 0; isys < n_sys; isys++) {
        const double delta = values.get(vid[isys]);
        const Histogram & t_sys = delta > 0 ? hplus[isys] : hminus[isys];
        if (t_sys.get_nbins() == 0)continue;
        h.multiply_with_ratio_exponented(t_sys, h0, fabs(delta));
    }
    h *= h0;
    return h;
}

const Histogram & InterpolatingHistogramFunction::gradient(const ParValues & values, const ParId & pid) const{
    const size_t n_sys = hplus.size();
    bool found = false;
    for (size_t isys = 0; isys < n_sys; isys++) {
        if(vid[isys]!=pid)continue;
        found = true;
        const double delta = values.get(vid[isys]);
        const Histogram & t_sys = delta > 0 ? hplus[isys] : hminus[isys];
        if (t_sys.get_nbins() == 0)continue;
        h.multiply_with_ratio_exponented(t_sys, h0, fabs(delta));
        const size_t nbins = t_sys.get_nbins();
        for(size_t i=1; i<=nbins; ++i){
            if(h0.get(i)>0.0)
                h.set(i, h.get(i) * theta::utils::log(t_sys.get(i) / h0.get(i)));
        }
        break;
    }
    if(not found){
        h.reset();
    }
    else{
        h *= h0;
    }
    return h;
}

theta::ParIds InterpolatingHistogramFunction::getParameters() const {
    theta::ParIds result;
    for (size_t i = 0; i < vid.size(); ++i) {
        result.insert(vid[i]);
    }
    return result;
}

std::auto_ptr<HistogramFunction> InterpolatingHistoFactory::build(ConfigurationContext & ctx) const{
    const Setting & psetting = ctx.setting["parameters"];
    vector<ParId> par_ids;
    vector<Histogram> hplus;
    vector<Histogram> hminus;
    //build nominal histogram:
    Histogram h0 = getConstantHistogram(ctx, psetting["nominal-histogram"]);
    int n = psetting.getLength();
    //note: allow n==0 to allow the user to disable systematics.
    // In case of unintentional type error (parameters="delta1,delta2";), user will get a warning about
    // the unused delta*-{plus,minus}-histogram blocks anyway ...
    for(int i=0; i<n; i++){
        string par_name = psetting[i];
        ParId pid = ctx.vm->getParId(par_name);
        par_ids.push_back(pid);
        stringstream setting_name;
        //plus:
        setting_name << par_name << "-plus-histogram";
        hplus.push_back(getConstantHistogram(ctx, ctx.setting[setting_name.str()]));
        //minus:
        setting_name.str("");
        setting_name << par_name << "-minus-histogram";
        hminus.push_back(getConstantHistogram(ctx, ctx.setting[setting_name.str()]));
    }
    assert(hplus.size()==hminus.size());
    assert(par_ids.size()==hminus.size());
    assert(static_cast<int>(par_ids.size())==n);
    ctx.rec.markAsUsed(psetting);
    //(other setting are not lowest-level and need not be marked).
    return std::auto_ptr<HistogramFunction>(new InterpolatingHistogramFunction(h0, par_ids, hplus, hminus));
}


Histogram InterpolatingHistoFactory::getConstantHistogram(ConfigurationContext & ctx, const libconfig::Setting & s){
    ConfigurationContext context(ctx, s);
    std::auto_ptr<HistogramFunction> hf = PluginManager<HistogramFunctionFactory>::get_instance()->build(context);
    if(hf->getParameters().size()!=0){
        stringstream ss;
        ss << "Histogram defined in path " << s.getPath() << " is not constant (but has to be).";
        throw InvalidArgumentException(ss.str());
    }
    return (*hf)(ParValues());//copies the internal reference, so this is ok.
}

REGISTER_FACTORY(InterpolatingHistoFactory)
