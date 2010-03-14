#include "interface/minimizer.hpp"
#include "interface/cfg-utils.hpp"
#include "interface/plugin_so_interface.hpp"
#include "libconfig/libconfig.h++"


#include <utility>
#include <sstream>

using namespace theta;
using namespace std;

void Minimizer::override_range(const theta::ParId & pid, double lower_limit, double upper_limit) {
    if (upper_limit < lower_limit) {
        std::stringstream s;
        s << "Minimizer::override_range (parameter: '" << vm->getName(pid)
                << "', lower=" << lower_limit << ", upper=" << upper_limit << "): invalid range";
        throw InvalidArgumentException(s.str());
    }
    overridden_ranges[pid] = std::make_pair(lower_limit, upper_limit);
}

void Minimizer::override_default(const theta::ParId & pid, double def){
    overridden_defaults[pid] = def;
}

void Minimizer::reset_override(const theta::ParId & pid) {
    overridden_ranges.erase(pid);
    overridden_defaults.erase(pid);
}

void Minimizer::set_initial_stepsize(const theta::ParId & pid, double stepsize) {
    if (stepsize <= 0) {
        std::stringstream s;
        s << "Minimizer::set_initial_stepsize (parameter: '" << vm->getName(pid)
                << "', stepsize=" << stepsize << "): step size mus be > 0";
        throw InvalidArgumentException(s.str());
    }
    initial_stepsizes[pid] = stepsize;
}

const std::pair<double, double> & Minimizer::get_range(const theta::ParId & pid) const {
    std::map<theta::ParId, std::pair<double, double> >::const_iterator it = overridden_ranges.find(pid);
    if (it != overridden_ranges.end()) return it->second;
    return vm->get_range(pid);
}

double Minimizer::get_default(const theta::ParId & pid) const{
    std::map<theta::ParId, double>::const_iterator it = overridden_defaults.find(pid);
    if (it != overridden_defaults.end()) return it->second;
    return vm->get_default(pid);
}

double Minimizer::get_initial_stepsize(const theta::ParId & pid) const {
    std::map<theta::ParId, double>::const_iterator it = initial_stepsizes.find(pid);
    if (it != initial_stepsizes.end()) return it->second;
    double def = fabs(vm->get_default(pid));
    if (def != 0) return 0.2 * def;
    const std::pair<double, double> & range = vm->get_range(pid);
    double interval_size = fabs(range.second - range.first); //fabs should not be necessary here, but who knows ...
    if (std::isinf(interval_size)) return 1.0;
    return 0.05 * interval_size;
}

/*
void theta::MinimizerUtils::apply_settings(Minimizer & m, const theta::plugin::Configuration & ctx){
    if(ctx.setting.exists("override-ranges")){
        SettingWrapper s_ranges = ctx.setting["override-ranges"];
        size_t size = s_ranges.size();
        for(size_t i=0; i<size; ++i){
            string parname = s_ranges[i].getName();
            double lower = s_ranges[i][0];
            double upper = s_ranges[i][1];
            m.override_range(ctx.vm->getParId(parname), lower, upper);
        }
    }

    if(ctx.setting.exists("initial-step-sizes")){
        SettingWrapper s_steps = ctx.setting["initial-step-sizes"];
        size_t size = s_steps.size();
        for(size_t i=0; i<size; ++i){
            string parname = s_steps[i].getName();
            double step = s_steps[i];
            m.set_initial_stepsize(ctx.vm->getParId(parname), step);
        }
    }
}
*/