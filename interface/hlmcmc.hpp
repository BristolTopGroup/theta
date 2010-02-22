#ifndef HLMCMC_H
#define HLMCMC_H

/* High level functions of Markov Chain Monte Carlo
including diagnostics and automatic choice of re-runs.


Implementation is based on the general-purpose algorithm described in:
Raftery, A.E. and Lewis, S.M. (1996). Implementing MCMC. 
In Markov Chain Monte Carlo in Practice(W.R. Gilks, D.J. Spiegelhalter and S. Richardson, eds.), 
London: Chapman and Hall, pp. 115-130.
(Obtainable at <http://www.stat.washington.edu/www/research/online/1994/raftery-lewis2.ps>)

It is particularly suited to estimate errors on quantiles of a posterior distribution.
The user input for this algorithm is:
- The quantile q to determine
- The acceptable error on q, r
- The confidence level s the error r on q, i.e. the coverage of the interval (q-r, q+r) on q_real
- The negative log likelihood function to marginalize
- An estimate on starting values (or intervals) 
- An estimate for the covariance matrix of the parameters (an estimate for the variance 
  of the individual parameters, i.e. a diagonal matrix, is good enough).

The algorithm runs several monte carlo chains until the estimate of q is believed
to be good enough.
At the moment, no sophisticated interactions with the algorithm is possible.
*/

#include "matrix.hpp"
#include "utils.hpp"
#include "mcmc-result.hpp"
#include "random.hpp"
#include "exception.hpp"
#include "metropolis.hpp"

#include <map>
#include <limits>
#include <boost/math/special_functions/fpclassify.hpp>
#include <iostream>

namespace theta{

/** \c ipar is the index of the parameter you want the quantile estimate of. */
template<class nlltype, class rndtype>
double rafteryLewisQuantile(size_t ipar, double q, double r, double s, const nlltype & nll,
    const std::vector<double> & startvalues, const Matrix & cov, rndtype & rnd);

/** some "private" functions used by rafteryLewis.
 * ipar, q, s and r are as above.
 */
static void calculate_mn(size_t ipar, double q, double s, double r,
   const FullResultMem & fr, double & m, double & n, double epsilon = -1.0); /* epsilon defaults to r/10 */

static double find_quantile(double q, double * list, size_t count, int lower, 
   int upper, double min, double max);

static double find_quantile(size_t ipar, double q, const FullResultMem & fr);

static double calcBIC(char* z, size_t count, size_t k);


template<class nlltype, class rndtype>
double rafteryLewisQuantile(size_t ipar, double q, double r, double s, const nlltype & nll,
    const std::vector<double> & startvalues, const Matrix & cov, rndtype & rnd){
   size_t npar = startvalues.size();
   if(npar != cov.getRows() || npar!= cov.getCols() || npar!=nll.getnpar())
       throw InvalidArgumentException("rafteryLewisQuantile: dimension/length of parameters mismatch");
   if(ipar>=npar) throw InvalidArgumentException("ERROR: ipar >= npar");
   double a = utils::phi_inverse((s+1)/2) / r;
   size_t N_min = static_cast<size_t>(a * a * q * (1 - q));
   /* Simulation 1.: make a run with N_min to determine pilot sample: */
   FullResultMem fr(npar, N_min);
   metropolisHastings<nlltype, FullResultMem, rndtype>(nll, fr, rnd, startvalues, cov, N_min, N_min);
   /* Simulation 2.: another run for getting the cov right this time: */
   Matrix our_cov(fr.getCov());
   fr.reset();
   metropolisHastings<nlltype, FullResultMem, rndtype>(nll, fr, rnd, startvalues, our_cov, N_min, N_min);;
   /* Simulation 3.--N.: calculate M and N with the information obtained so far
    * and use those values for the subsequent runs. Continue until we have more
    * iterations thgan suggested (burn-in is discarded in the moment). */
   double m, n, lastm, lastn;
   m=n=lastm=lastn=N_min;
   size_t z = 0;
   do{
      our_cov = fr.getCov();
      lastm = m;
      lastn = n;
      double tmp_m, tmp_n;
      calculate_mn(ipar, q, s, r, fr, tmp_m, tmp_n);
      if((boost::math::isnan)(tmp_m) || (boost::math::isnan)(tmp_n)){
          m *= 2;
          n *= 2;
      }
      else{
          m=tmp_m;
          n=tmp_n;
          if(lastn>n)break;
      }
      fr.reset();
      metropolisHastings<nlltype, FullResultMem,rndtype>(nll, fr, rnd, startvalues,
                our_cov, static_cast<size_t>(n*2), static_cast<size_t>(m*2));
      z++;
   }while(z < 10 && lastn < n);
   if(z>=10) return std::numeric_limits<double>::signaling_NaN();
   /* else determine the quantile of the last run and return it: */
   return find_quantile(ipar, q, fr);
}

}

#endif
