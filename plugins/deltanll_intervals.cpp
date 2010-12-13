#include "plugins/deltanll_intervals.hpp"
#include "plugins/reduced_nll.hpp"
#include "interface/plugin.hpp"
#include "interface/run.hpp"
#include "interface/minimizer.hpp"
#include "interface/histogram.hpp"
#include "interface/distribution.hpp"

#include <sstream>

using namespace theta;
using namespace std;
using namespace libconfig;

void deltanll_intervals::define_table(){
    c_maxl = table->add_column(*this, "maxl", Table::typeDouble);
    for(size_t i=0; i<clevels.size(); ++i){
        stringstream ss;
        ss << "lower" << setw(5) << setfill('0') << static_cast<int>(clevels[i] * 10000 + 0.5);
        lower_columns.push_back(table->add_column(*this, ss.str(), Table::typeDouble));
        ss.str("");
        ss << "upper" << setw(5) << setfill('0') << static_cast<int>(clevels[i] * 10000 + 0.5);
        upper_columns.push_back(table->add_column(*this, ss.str(), Table::typeDouble));
    }
}


/** \brief The secant method to find the root of a one-dimensional function
 *
 * \param x_low The lower end of the start interval
 * \param x_high The higher end of the start interval
 * \param x_accuracy If the found interval is shorter than this, the iteration will stop
 * \param f_x_low is function(x_low).Used to save one function evalutation.
 * \param f_x_high is function(x_high). Used to save one function evaluation.
 * \param function The function object to use.
 *
 * Note that the function values at x_low and x_high must have different sign. Otherwise,
 * an InvalidArgumentException will be thrown.
 */
template<typename T>
double secant(double x_low, double x_high, double x_accuracy, double f_x_low, double f_x_high, const T & function){
    assert(x_low <= x_high);
    if(f_x_low * f_x_high >= 0) throw InvalidArgumentException("secant: function values have the same sign!");
    
    const double old_interval_length = x_high - x_low;
    
    //calculate intersection point for secant method:
    double x_intersect = x_low - (x_high - x_low) / (f_x_high - f_x_low) * f_x_low;
    assert(x_intersect >= x_low);
    assert(x_intersect <= x_high);
    if(old_interval_length < x_accuracy){
        return x_intersect;
    }
    double f_x_intersect = function(x_intersect);
    double f_mult = f_x_low * f_x_intersect;
    //fall back to bisection if the new interval would not be much smaller:
    double new_interval_length = f_mult < 0 ? x_intersect - x_low : x_high - x_intersect;
    if(new_interval_length > 0.5 * old_interval_length){
        x_intersect = 0.5*(x_low + x_high);
        f_x_intersect = function(x_intersect);
        f_mult = f_x_low * f_x_intersect;
    }
    if(f_mult < 0){
        return secant(x_low, x_intersect, x_accuracy, f_x_low, f_x_intersect, function);
    }
    else if(f_mult > 0.0){
        return secant(x_intersect, x_high, x_accuracy, f_x_intersect, f_x_high, function);
    }
    //it can actually happen that we have 0.0. In this case, return the x value for
    // the smallest absolute function value (and prevent testing against absolue 0.0).
    else{
        f_x_intersect = fabs(f_x_intersect);
        f_x_low = fabs(f_x_low);
        f_x_high = fabs(f_x_high);
        if(f_x_low < f_x_high && f_x_low < f_x_intersect) return x_low;
        if(f_x_high < f_x_intersect) return x_high;
        return x_intersect;
    }
}


void deltanll_intervals::produce(theta::Run & run, const theta::Data & data, const theta::Model & model) {
    std::auto_ptr<NLLikelihood> nll = get_nllikelihood(data, model);
    if(not start_step_ranges_init){
        const Distribution & d = nll->get_parameter_distribution();
        DistributionUtils::fillModeWidthSupport(start, step, ranges, d);
        start_step_ranges_init = true;
    }
    
    MinimizationResult minres = minimizer->minimize(*nll, start, step, ranges);
    
    const double value_at_minimum = minres.values.get(pid);
    table->set_column(*c_maxl, value_at_minimum);
    
    
    ReducedNLL nll_r(*nll, pid, minres.values, re_minimize ? minimizer.get() : 0, start, step, ranges);
    
    const pair<double, double> & range = ranges[pid];
    for(size_t i=0; i < deltanll_levels.size(); ++i){
        nll_r.set_offset_nll(minres.fval + deltanll_levels[i]);
        //upper value: look for a parameter value with a sign flip:
        double x_low = value_at_minimum;
        double f_x_low = -deltanll_levels[i];
        double initial_step = minres.errors_plus.get(pid);
        if(initial_step <= 1e-6 * fabs(x_low)) initial_step = 1e-6 * fabs(x_low);
        if(initial_step < 1e-6) initial_step = 1e-6;
        double step = initial_step;
        const double x_acurracy = step / 100;
        double x_high, f_x_high;
        int k;
        const int k_max = 20;
        for(k=1; k<=k_max; ++k){
            x_high = value_at_minimum + step;
            step *= 2;
            if(x_high > range.second){
                x_high = range.second;
            }
            f_x_high = nll_r(x_high);
            if(f_x_high > 0){
                table->set_column(upper_columns[i], secant(x_low, x_high, x_acurracy, f_x_low, f_x_high, nll_r));
                break;
            }
            else if(f_x_high==0.0 || x_high == range.second){
                table->set_column(upper_columns[i], x_high);
                break;
            }
            else{
                x_low = x_high;
                f_x_low = f_x_high;
            }
        }
        if(k > k_max){
            throw Exception("could not find upper value for interval");
        }
        
        //lower value: same story, just other way round:
        x_high = value_at_minimum;
        f_x_high = -deltanll_levels[i];
        step = initial_step;
        if(step <= 0.0) step = 1.0;
        for(k=1; k <= k_max; ++k){
            x_low = value_at_minimum - step;
            step *= 2;
            if(x_low < range.first){
                x_low = range.first;
            }
            f_x_low = nll_r(x_low);
            if(f_x_low > 0){
                table->set_column(lower_columns[i], secant(x_low, x_high, x_acurracy, f_x_low, f_x_high, nll_r));
                break;
            }
            else if(f_x_low==0.0 || x_low == range.first){
                table->set_column(lower_columns[i], x_low);
                break;
            }
            else{
                x_high = x_low;
                f_x_high = f_x_low;
            }
        }
        if(k > k_max){
            throw Exception("could not find lower value for interval");
        }
    }
}

deltanll_intervals::deltanll_intervals(const theta::plugin::Configuration & cfg): Producer(cfg),
        re_minimize(true), start_step_ranges_init(false){
    SettingWrapper s = cfg.setting;
    minimizer = theta::plugin::PluginManager<Minimizer>::instance().build(theta::plugin::Configuration(cfg, s["minimizer"]));
    string par_name = s["parameter"];
    pid = cfg.vm->getParId(par_name);
    size_t ic = s["clevels"].size();
    if (ic == 0) {
        throw ConfigurationException("deltanll_intervals: empty clevels.");
    }
    for (size_t i = 0; i < ic; i++) {
        clevels.push_back(s["clevels"][i]);
    }
    if(s.exists("re-minimize")){
        re_minimize = s["re-minimize"];
    }
    deltanll_levels.resize(clevels.size());
    for(size_t i=0; i<clevels.size(); ++i){
        if(clevels[i] < 0.0) throw InvalidArgumentException("deltanll_intervals: clevel < 0 not allowed.");
        if(clevels[i] >= 1.0) throw InvalidArgumentException("deltanll_intervals: clevel >= 1.0 not allowed.");
        deltanll_levels[i] = utils::phi_inverse((1+clevels[i])/2);
        deltanll_levels[i] *= deltanll_levels[i]*0.5;
    }
}

REGISTER_PLUGIN(deltanll_intervals)
