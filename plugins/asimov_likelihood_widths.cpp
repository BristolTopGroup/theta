#include "plugins/asimov_likelihood_widths.hpp"
#include "plugins/secant.hpp"
#include "interface/distribution.hpp"
#include "interface/model.hpp"
#include "interface/matrix.hpp"
#include "interface/exception.hpp"

#include <sstream>


using namespace theta;
using namespace std;

namespace{

//function object depending on one double
// which evaluates the nll using parameter values at some given point
// except one parameter. Also subtracts value at the mode such that
// the value there is 0.
struct nll_mode_pid{
   nll_mode_pid(const ParValues & mode_, const ParId & pid_, const Function & f_, double subtract_value_): values(mode_), pid(pid_),
      subtract_value(subtract_value_), f(f_){
   }
   
   double operator()(double p) const{
       values.set(pid, p);
       return f(values) - subtract_value;
   }
private:
   mutable ParValues values;
   const ParId pid;
   double subtract_value;
   const Function & f;
};


std::auto_ptr<NLLikelihood> get_asimov_nllikelihood(const theta::Model & model, const boost::shared_ptr<Distribution> & override_parameter_distribution, const boost::shared_ptr<theta::Function> & additional_nll_term){
const Distribution & dist = override_parameter_distribution.get()? *override_parameter_distribution: model.get_parameter_distribution();
    ParIds parameters = model.get_parameters();
    ParValues mode;
    dist.mode(mode);
    Data asimov_data;
    model.get_prediction(asimov_data, mode);
    
    const Distribution * rvobs_dist = model.get_rvobservable_distribution();
    if(rvobs_dist){
        rvobs_dist->mode(mode);
        asimov_data.set_rvobs_values(ParValues(mode, model.get_rvobservables()));
    }
    std::auto_ptr<NLLikelihood> nll = model.get_nllikelihood(asimov_data);
    //0 value has same semantics for NLLikelihood:
    nll->set_override_distribution(override_parameter_distribution);
    nll->set_additional_term(additional_nll_term);
    if(additional_nll_term.get()){
        parameters.insert_all(additional_nll_term->get_parameters());
    }
    theta_assert(dist.get_parameters() == parameters);
    return nll;
}

ParValues get_widths(const Distribution & dist, const theta::Function & nll){
    ParValues mode;
    dist.mode(mode);
    const ParIds & parameters = nll.get_parameters();
    theta_assert(mode.contains_all(parameters));
    double nll_at_min = nll(mode);
    ParValues result;
    int k=0;
    for(ParIds::const_iterator it=parameters.begin(); it!=parameters.end(); ++it, ++k){
        ParId pid = *it;
        const double pid_mode = mode.get(pid);
        std::pair<double, double> support = dist.support(pid);
        theta_assert(support.first <= pid_mode && pid_mode <= support.second);
        if(support.first == support.second){
            result.set(pid, 0.0);
            continue;
        }
        nll_mode_pid f(mode, pid, nll, nll_at_min + 0.5);
        //if one end is finite, try to use it. Save whether the interval end is considered
        // "fl0", i.e. the interval end itself is finite but the function value there is invalid (< 0).
        bool low_is_fl0 = false, high_is_fl0 = false;
        if(std::isfinite(support.second)){
            double f2 = f(support.second);
            if(f2==0.0){
                result.set(pid, fabs(pid_mode - support.second));
                continue;
            }
            if(!std::isfinite(f2) || f2 < 0){
               low_is_fl0 = true;
            }
            else{
               result.set(pid, fabs(pid_mode - secant(pid_mode, support.second, 0.0, -0.5, f2, 0.05, f)));
               continue;
            }
        }
        if(std::isfinite(support.first)){
            double f2 = f(support.first);
            if(f2==0.0){
                result.set(pid, fabs(pid_mode - support.first));
                continue;
            }
            if(!std::isfinite(f2) || f2 < 0){
               high_is_fl0 = true;
            }
            else{
               result.set(pid, fabs(pid_mode - secant(support.first, pid_mode, 0.0, f2, -0.5, 0.05, f)));
               continue;
            }
        }
        //the support was either infinite or the values at the borders were not sufficiently high.
        // Treat second case first:
        if(low_is_fl0 && high_is_fl0){
            result.set(pid, support.second - support.first);
            continue;
        }
        //Now, one of the interval ends has to be infinite, otherwise we would not be here.
        //Scan in that direction:
        theta_assert(std::isinf(support.first) || std::isinf(support.second));
        bool found = false;
        for(double sign = -1.0; sign <= 1.001; sign+=2.0){
            if(!std::isinf(support.first) && sign < 0) continue;
            if(!std::isinf(support.second) && sign > 0) continue;
            // as step size, try the parameter value, if it is not zero:
            double step = fabs(pid_mode);
            if(step==0) step = 1.0;
            for(int i=0; i<1000; ++i){
                double fval = f(pid_mode + sign * step);
                if(std::isinf(fval)){
                    step /= 1.5;
                    continue;
                }
                step *= 2.0;
                if(fval > 0){
                    double xlow, xhigh, flow, fhigh;
                    xlow = pid_mode; flow = -0.5;
                    xhigh = pid_mode + sign * step; fhigh = fval;
                    if(sign < 0){
                        std::swap(xlow, xhigh);
                        std::swap(flow, fhigh);
                    }
                    theta_assert(xlow <= xhigh);
                    result.set(pid, fabs(pid_mode - secant(xlow, xhigh, 0.0, flow, fhigh, 0.05, f)));
                    found = true;
                    break;
                }
            }
            if(found) break;
        }
        if(found) continue;
        stringstream ss;
        ss << "asimov_likelihood_widths: could not find width for parameter " << pid;
        throw Exception(ss.str());
    }
    return result;
}


class Ranges{
   map<ParId, pair<double, double> > ranges_;
public:
   explicit Ranges(const map<ParId, pair<double, double> > & ranges): ranges_(ranges){
      map<ParId, pair<double, double> >::const_iterator it = ranges.begin();
      for(; it!=ranges.end(); ++it){
         theta_assert(it->second.first <= it->second.second);
      }
   }
   
   explicit Ranges(const theta::Distribution & dist){
       const ParIds & pars = dist.get_parameters();
       for(ParIds::const_iterator it=pars.begin(); it!=pars.end(); ++it){
           ranges_[*it] = dist.support(*it);
           theta_assert(ranges_[*it].first <= ranges_[*it].second);
       }
   }

   pair<double, double> get(const ParId & pid) const{
      map<ParId, pair<double, double> >::const_iterator it = ranges_.find(pid);
      if(it==ranges_.end()){
         throw invalid_argument("no range set for this parid");
      }
      return it->second;
   }

   bool fixed(const ParId & pid) const{
      map<ParId, pair<double, double> >::const_iterator it = ranges_.find(pid);
      if(it==ranges_.end()){
         throw invalid_argument("no range set for this parid");
      }
      return it->second.first == it->second.second;
   }

   // truncate the values to the ranges.
   void trunc(ParValues & values) const{
      for(map<ParId, pair<double, double> >::const_iterator it=ranges_.begin(); it!=ranges_.end(); ++it){
         if(!values.contains(it->first)) continue;
         double val = values.get(it->first);
         if(val < it->second.first) val = it->second.first;
         else if(val > it->second.second) val = it->second.second;
         values.set(it->first, val);
      }
   }
};


const double eps = std::numeric_limits<double>::epsilon();
const double s_eps = sqrt(eps);

// evaluate function, respecting the parameter ranges
// values must be within the range. Does not update grad for fixed parameters.
//
// n = 1 means to calculate
// f'(x) = f(x + h) - f(x) / h    or   f'(x) = f(x) - f(x-h) / h    in case x+h is out of range.
// n=2 calculates:
// f'(x) = f(x+h) - f(x-h) / 2h
//
// in the latter case: if x+h or x-h is out of range, the n=1 case is used.
double eval_with_grad(const Function & f, const Ranges & ranges, const ParValues & values, const ParValues & step, ParValues & grad, int n){
    const ParIds & pids = f.get_parameters();
    const double f0 = f(values);
    theta_assert(n==1 or n==2);
    for(ParIds::const_iterator it=pids.begin(); it!=pids.end(); ++it){
        const ParId & pid = *it;
        if(ranges.fixed(pid)) continue;
        pair<double, double> range = ranges.get(pid);
        double x0 = values.get(pid);
        double s = step.get(pid);
        theta_assert(s > 0.0);
        double x0_plus = x0 + s * s_eps;
        double x0_minus = x0 - s * s_eps;
        ParValues vals(values);
        if(x0_plus > range.second){
            x0_plus = x0;
        }
        if(x0_minus < range.first){
            x0_minus = x0;
        }
        double fplus, fminus;
        volatile double h;
        if(n==2 and x0_minus != x0 and x0_plus != x0){
            h = x0_plus - x0_minus;
            vals.set(pid, x0_plus);
            fplus = f(vals);
            vals.set(pid, x0_minus);
            fminus = f(vals);
        }
        else{
            if(x0_plus != x0){
                h = x0_plus - x0;
                vals.set(pid, x0_plus);
                fplus = f(vals);
                fminus = f0;
            }
            else if(x0_minus != x0){
                h = x0 - x0_minus;
                vals.set(pid, x0_minus);
                fminus = f(vals);
                fplus = f0;
            }
            else{
                throw invalid_argument("relative parameter range is to small");
            }
        }
        grad.set(pid, (fplus - fminus) / h);
    }
    return f0;
}



} // anon. namespace



Matrix asimov_likelihood_matrix(const theta::Model & model, const boost::shared_ptr<Distribution> & override_parameter_distribution,
                                const boost::shared_ptr<theta::Function> & additional_nll_term){
    std::auto_ptr<NLLikelihood> nll = get_asimov_nllikelihood(model, override_parameter_distribution, additional_nll_term);
    const Distribution & dist = override_parameter_distribution.get()? *override_parameter_distribution: model.get_parameter_distribution();
    const ParIds & parameters = nll->get_parameters();
    const Ranges ranges(dist);
    Matrix result(parameters.size(), parameters.size());
    ParValues mode;
    dist.mode(mode);
    ParValues widths = get_widths(dist, *nll);
    ParValues g0;
    eval_with_grad(*nll, ranges, mode, widths, g0, 2);
    size_t i=0;
    for(ParIds::const_iterator it=parameters.begin(); it!=parameters.end(); ++it, ++i){
        const ParId & pid = *it;
        if(ranges.fixed(pid)) continue; // and leave matrix at 0
        pair<double, double> range = ranges.get(pid);
        double x0 = mode.get(pid);
        double s = widths.get(pid);
        theta_assert(s > 0.0);
        double x0_plus = x0 + s * s_eps;
        double x0_minus = x0 - s * s_eps;
        if(x0_plus > range.second){
            x0_plus = x0;
        }
        if(x0_minus < range.first){
            x0_minus = x0;
        }
        ParValues g_minus, g_plus;
        volatile double h;
        ParValues vals(mode);
        if(x0_minus != x0 and x0_plus != x0){
            h = x0_plus - x0_minus;
            vals.set(pid, x0_plus);
            eval_with_grad(*nll, ranges, vals, widths, g_plus, 2);
            vals.set(pid, x0_minus);
            eval_with_grad(*nll, ranges, vals, widths, g_minus, 2);
        }
        else{
            if(x0_plus != x0){
                h = x0_plus - x0;
                vals.set(pid, x0_plus);
                eval_with_grad(*nll, ranges, vals, widths, g_plus, 2);
                g_minus = g0;
            }
            else if(x0_minus != x0){
                h = x0 - x0_minus;                
                vals.set(pid, x0_minus);
                eval_with_grad(*nll, ranges, vals, widths, g_minus, 2);
                g_plus = g0;
            }
            else{
                throw invalid_argument("relative parameter range is to small");
            }
        }
        // set matrix:
        size_t j=0;
        for(ParIds::const_iterator it2=parameters.begin(); it2!=parameters.end(); ++it2, ++j){
            //result(i,j) = (g_plus.get(*it2) - g_minus.get(*it2)) / h;
            double fpp;
            result(i,j) += (fpp = 0.5 * (g_plus.get(*it2) - g_minus.get(*it2)) / h);
            result(j,i) += fpp;
        }
    }
    // covariance is the inverse of the hessian:
    result.invert_cholesky();
    return result;
}


ParValues asimov_likelihood_widths(const theta::Model & model, const boost::shared_ptr<Distribution> & override_parameter_distribution,
                                   const boost::shared_ptr<theta::Function> & additional_nll_term){
    const Distribution & dist = override_parameter_distribution.get()? *override_parameter_distribution: model.get_parameter_distribution();
    std::auto_ptr<NLLikelihood> nll = get_asimov_nllikelihood(model, override_parameter_distribution, additional_nll_term);
    return get_widths(dist, *nll);
}


