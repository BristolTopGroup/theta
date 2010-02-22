#include "interface/minimizer.hpp"
#include "interface/plugin_so_interface.hpp"

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

void Minimizer::reset_range_override(const theta::ParId & pid) {
    overridden_ranges.erase(pid);
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

std::pair<double, double> Minimizer::get_range(const theta::ParId & pid) const {
    std::map<theta::ParId, std::pair<double, double> >::const_iterator it = overridden_ranges.find(pid);
    if (it != overridden_ranges.end()) return it->second;
    return vm->get_range(pid);
}

double Minimizer::get_initial_stepsize(const theta::ParId & pid) const {
    std::map<theta::ParId, double>::const_iterator it = initial_stepsizes.find(pid);
    if (it != initial_stepsizes.end()) return it->second;
    double def = fabs(vm->get_default(pid));
    if (def != 0) return 0.2 * def;
    std::pair<double, double> range = vm->get_range(pid);
    double interval_size = fabs(range.second - range.first); //fabs should not be necessary here, but who knows ...
    if (std::isinf(interval_size)) return 1.0;
    return 0.05 * interval_size;
}


void theta::MinimizerUtils::apply_settings(Minimizer & m, theta::plugin::ConfigurationContext & ctx){
    if(ctx.setting.exists("override-ranges")){
        const libconfig::Setting & s_ranges = ctx.setting["override-ranges"];
        if(not s_ranges.isGroup()){
            stringstream s;
            s << "Setting '" << s_ranges.getPath() << "' must be a setting group.";
            throw ConfigurationException(s.str());
        }
        int size = s_ranges.getLength();
        for(int i=0; i<size; ++i){
            string parname = s_ranges[i].getName();
            double lower = s_ranges[i][0];
            double upper = s_ranges[i][1];
            ctx.rec.markAsUsed(s_ranges[i]);
            m.override_range(ctx.vm->getParId(parname), lower, upper);
        }
    }

    if(ctx.setting.exists("initial-step-sizes")){
        const libconfig::Setting & s_steps = ctx.setting["initial-step-sizes"];
        if(not s_steps.isGroup()){
            stringstream s;
            s << "Setting '" << s_steps.getPath() << "' must be a setting group.";
            throw ConfigurationException(s.str());
        }
        int size = s_steps.getLength();
        for(int i=0; i<size; ++i){
            string parname = s_steps[i].getName();
            double step = s_steps[i];
            ctx.rec.markAsUsed(s_steps[i]);
            m.set_initial_stepsize(ctx.vm->getParId(parname), step);
        }
    }


}


