#ifndef PLUGINS_REDUCED_NLL_HPP
#define PLUGINS_REDUCED_NLL_HPP

#include "interface/minimizer.hpp"
#include "interface/variables.hpp"
#include "interface/phys.hpp"

#include <sstream>

/** \brief Reduced likelihood function
 *
 * This class is used by several plugins based on the reduiced likelihood function, e.g.,
 * \ref deltanll_intervals and \ref nll_scan. See the documentation of \ref deltanll_intervals
 * of a definition of the term "reduced likelihood function", including the variations
 * with and without minimization.
 *
 */
class ReducedNLL{
    public:
        
        /** \brief Construct a reduced likelihood function
         *
         * Set the Minimizer \c min_ to zero if no minimization should be done.
         */
        ReducedNLL(const theta::NLLikelihood & nll_, const theta::ParId & pid_, const theta::ParValues & pars_at_min_, theta::Minimizer * min_):
           nll(nll_), pid(pid_), pars_at_min(pars_at_min_), min(min_){
        }
        
        /** \brief Set the offset to subtract from the value of the likelihood function
         *
         * See operator()
         */
        void set_offset_nll(double t){
            offset_nll = t;
        }
        
        /** \brief Evaluate the reduced likelihood function at x
         *
         * If the minimizer was not the NULL-pointer, a new minimization is done
         * setting the parameter to x and minimizing the nll w.r.t. all other parameters, ignoring
         * the parameter values at the minimum.
         *
         * Otherwise, the parameter values at the minimum are used, just the parameter of interest is set to x.
         */
        double operator()(double x) const{
            if(min){
                min->override_default(pid, x);
                min->override_range(pid, x, x);
                theta::MinimizationResult minres = min->minimize(nll);
                return minres.fval - offset_nll;
            }
            pars_at_min.set(pid, x);
            return nll(pars_at_min) - offset_nll;
        }
        
        /** \brief Destructor
         *
         * Releases parameter overrides set for the parameter of interest.
         */
        ~ReducedNLL(){
            if(min) min->reset_override(pid);
        }
        
    private:
        const theta::NLLikelihood & nll;
        theta::ParId pid;
        mutable theta::ParValues pars_at_min;
        double offset_nll;
        theta::Minimizer * min;
};


#endif

