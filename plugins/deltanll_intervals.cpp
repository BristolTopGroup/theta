#include "plugins/deltanll_intervals.hpp"
#include "interface/plugin.hpp"
#include "interface/run.hpp"
#include "interface/minimizer.hpp"
#include "interface/histogram.hpp"

#include <sstream>

using namespace theta;
using namespace std;
using namespace libconfig;

void deltanll_intervals::define_table(){
    c_maxl = table->add_column(*this, "maxl", ProducerTable::typeDouble);
    for(size_t i=0; i<clevels.size(); ++i){
        stringstream ss;
        ss << "lower" << setw(5) << setfill('0') << static_cast<int>(clevels[i] * 10000 + 0.5);
        lower_columns.push_back(table->add_column(*this, ss.str(), ProducerTable::typeDouble));
        ss.str("");
        ss << "upper" << setw(5) << setfill('0') << static_cast<int>(clevels[i] * 10000 + 0.5);
        upper_columns.push_back(table->add_column(*this, ss.str(), ProducerTable::typeDouble));
    }
}

class ReducedNLL{
    public:
        
        // set min to zero if no minimization should be done.
        ReducedNLL(const theta::NLLikelihood & nll_, const ParId & pid_, const ParValues & pars_at_min_, theta::Minimizer * min_):
           nll(nll_), pid(pid_), pars_at_min(pars_at_min_), min(min_){
        }
        
        void set_offset_nll(double t){
            offset_nll = t;
        }
        
        double operator()(double x) const{
            //use the last minimum to set the start values (empty at the first iteration, which is Ok).
            /*ParIds ids = last_minimum.getAllParIds();
            for(ParIds::const_iterator it=ids.begin(); it!=ids.end(); ++it){
                min->override_default(*it, last_minimum.get(*it));
            }*/
            if(min){
                min->override_default(pid, x);
                min->override_range(pid, x, x);
                MinimizationResult minres = min->minimize(nll);
                //last_minimum = minres.values;
                return minres.fval - offset_nll;
            }
            pars_at_min.set(pid, x);
            return nll(pars_at_min) - offset_nll;
        }
        
        ~ReducedNLL(){
            /*ParIds ids = last_minimum.getAllParIds();
            for(ParIds::const_iterator it=ids.begin(); it!=ids.end(); ++it){
                min->reset_override(*it);
            }*/
            if(min) min->reset_override(pid);
        }
        
    private:
        const theta::NLLikelihood & nll;
        ParId pid;
        mutable ParValues pars_at_min;
        double offset_nll;
        theta::Minimizer * min;
        //mutable ParValues last_minimum;
};


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
    if(f_x_low * f_x_high >= 0) throw InvalidArgumentException("secant: function values have the same sign!");
    double old_interval_length = x_high - x_low;
    assert(x_low <= x_high);
    double df = (f_x_high - f_x_low);
    if(utils::close_to(f_x_low, 0, df)){
        return x_low;
    }
    double x_intersect = x_low - (x_high - x_low) / df * f_x_low;
    assert(x_intersect > x_low);
    assert(x_intersect < x_high);
    if(x_high - x_low < x_accuracy){
        return x_intersect;
    }
    double f_x_intersect = function(x_intersect);
    //printf("    [%.5f, %.5f] --> [%.5f, %.5f];   new point %.5f --> %.5f", x_low, x_high, f_x_low, f_x_high, x_intersect, f_x_intersect);
    bool lower = f_x_low * f_x_intersect < 0;
    //fall back to bisection if the new interval would not be much smaller:
    double new_interval_length = lower ? x_intersect - x_low : x_high - x_intersect;
    if(new_interval_length > 0.5 * old_interval_length){
        x_intersect = 0.5*(x_low + x_high);
        f_x_intersect = function(x_intersect);
        lower = f_x_low * f_x_intersect < 0;
        //printf("; bisecting: %.5f --> %.5f\n", x_intersect, f_x_intersect);
    }
    else{
        //printf("\n");
    }
    if(lower){
        return secant(x_low, x_intersect, x_accuracy, f_x_low, f_x_intersect, function);
    }
    else{
        return secant(x_intersect, x_high, x_accuracy, f_x_intersect, f_x_high, function);
    }
}


void deltanll_intervals::produce(Run & run, const Data & data, const Model & model) {
    NLLikelihood nll = model.getNLLikelihood(data);
    MinimizationResult minres = minimizer->minimize(nll);
    
    const double value_at_minimum = minres.values.get(pid);
    table->set_column(c_maxl, value_at_minimum);
    const pair<double, double> & range = vm->get_range(pid);
    
    ReducedNLL nll_r(nll, pid, minres.values, re_minimize ? minimizer.get() : 0);
    
    for(size_t i=0; i < deltanll_levels.size(); ++i){
        nll_r.set_offset_nll(minres.fval + deltanll_levels[i]);
        //upper value: look for a parameter value with a sign flip:
        double x_low = value_at_minimum;
        double f_x_low = -deltanll_levels[i];
        double step = minres.errors_plus.get(pid);
        if(step <= 0.0) step = 1.0;
        const double x_acurracy = step / 100;
        double x_high, f_x_high;
        int k;
        //printf("scanning high; low =  [%.5f] --> %.5f\n", x_low, f_x_low);
        for(k=1; k<=20; ++k){
            x_high = value_at_minimum + step;
            step *= 2;
            if(x_high > range.second){
                x_high = range.second;
            }
            f_x_high = nll_r(x_high);
          //  printf("     hp [%.5f] --> %.5f\n", x_high, f_x_high);
            if(f_x_high > 0){
                table->set_column(upper_columns[i], secant(x_low, x_high, x_acurracy, f_x_low, f_x_high, nll_r));
                break;
            }
            else if(x_high == range.second){
                table->set_column(upper_columns[i], x_high);
                break;
            }
            else{
                x_low = x_high;
                f_x_low = f_x_high;
            }
        }
        if(k==11){
            throw Exception("could not find upper value for interval");
        }
        
        //lower value: same story, just other way round:
        x_high = value_at_minimum;
        f_x_high = -deltanll_levels[i];
        step = minres.errors_minus.get(pid);
        if(step <= 0.0) step = 1.0;
        for(k=1; k<=20; ++k){
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
            else if(x_low == range.first){
                table->set_column(lower_columns[i], x_low);
                break;
            }
            else{
                x_high = x_low;
                f_x_high = f_x_low;
            }
        }
        if(k==11){
            throw Exception("could not find lower value for interval");
        }
    }
}

deltanll_intervals::deltanll_intervals(const theta::plugin::Configuration & cfg): Producer(cfg), vm(cfg.vm), re_minimize(true){
    SettingWrapper s = cfg.setting;
    minimizer = theta::plugin::PluginManager<Minimizer>::build(theta::plugin::Configuration(cfg, s["minimizer"]));
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
        deltanll_levels[i] = utils::phi_inverse(1-(1-clevels[i])/2);
        deltanll_levels[i] *= deltanll_levels[i]*0.5;
    }
}

REGISTER_PLUGIN(deltanll_intervals)
