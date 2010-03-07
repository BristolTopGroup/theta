#ifndef THETA_MCMC_HPP
#define THETA_MCMC_HPP

#include "interface/matrix.hpp"
#include "interface/exception.hpp"
#include "interface/random.hpp"
#include "interface/phys.hpp"

#include <vector>
#include <cmath>

#include <boost/scoped_array.hpp>

namespace theta {
    /** Run the metropolis-Hastings Markov-Chain Monte-Carlo algorithm.
     *
     * @param nllikelihood is a function object which must implement double operator()(const double*) const which returns
     *  the negative logarithm of the likelihood (rather: the posterior) to integrate.
     * @param res is a "container" where the Markov Chain will be saved. It must implement the methods:
     * <ul>
     *   <li><tt>size_t getnpar()</tt> the number of parameters this result object was constructed for (required here
     *      for consistency checks only)</li>
     *   <li><tt>void fill(const double * x, double nll, size_t n)</tt>: this function will be called to indicate that the
     *       chain has <tt>n</tt> points at parameter values <tt>x</tt> and negative logarithm value of <tt>nll</tt>. What
     *       the result object does with this information is not of our concern here. Typically, it will either save the complete
     *       chain or fill the x values into a histogram</li>
     *   <li><tt>void end()</tt> is called after the chain is complete. Any cleanup necessary can be done here.
     * </ul>
     * @param rand is the random number generator to use to compute the next candidate point of the chain.
     *   It has to implement the <tt>double gauss()</tt> and <tt>double uniform()</tt>
     *   methods which shall return a random number distributed according to a gaussian around 0 with width 1 and a uniform distribution
     *   in the intervall [0, 1], respectively (whether or not the endpoints are actually included in the uniform case does not matter here).
     * @param startvalues is the point where to start the Markov Chain.
     * @param sqrt_cov is (apart from a overall factor) the matrix used to transform the uniform-Gaussian
     *   jumping kernel before adding it to the current point. It should be set to the Cholesky decomposition of the
     *   covariance matrix of the likelihood (or an approximation thereof), hence the name "square root of covariance".
     * @param iterations is the number of iterations for the Markov Chain which will be reported to the \c res object.
     * @param burn_in is the number of Markov-Chain iterations run at the beginning which are <em>not</em> reported to the \c res
     *   object.
     */
    template<class nlltype, class resulttype>
    void metropolisHastings(const nlltype & nllikelihood, resulttype &res, Random & rand,
            const std::vector<double> & startvalues, const Matrix & sqrt_cov, size_t iterations, size_t burn_in) {
    const size_t npar = startvalues.size();
    if(npar != sqrt_cov.getRows() || npar!=sqrt_cov.getCols() || npar!=nllikelihood.getnpar() || npar!=res.getnpar())
        throw InvalidArgumentException("metropolisHastings: dimension/size of arguments mismatch!");
    size_t npar_reduced = npar;
    for(size_t i=0; i<npar; i++){
       if(sqrt_cov(i,i)==0) npar_reduced--;
    }
    double factor = 2.38 / sqrt(npar_reduced);

        //keep the lower triangle   L    of the sqrt_cov to multiply that with deltaX:
        boost::scoped_array<double> lm(new double[npar * (npar + 1) / 2]);
        size_t z = 0;
        for (size_t i = 0; i < npar; i++) {
            for (size_t j = 0; j <= i; j++) {
                lm[z] = sqrt_cov(i,j) * factor;
                z++;
            }
        }

        //0. initialization:
        boost::scoped_array<double> x(new double[npar]);
        boost::scoped_array<double> x_new(new double[npar]);
        boost::scoped_array<double> dx(new double[npar]);
        //set the starting point:
        std::copy(startvalues.begin(), startvalues.end(), &x[0]);
        double nll = nllikelihood(x.get());

        size_t iter = burn_in + iterations;
        size_t weight = 1;
        //note: splitting up this for-loop
        // into two loops (one for burn-in, one for
        // recording) seems to save some time for the saved if(it>=burn_in)
        // but it is actually slower ... (tested with gcc 4.3.3, -O3).
        for (size_t it = 1; it < iter; it++) {
            for (size_t i = 0; i < npar; i++) {
                dx[i] = rand.gauss();
            }
            //multiply lm with dx and add to x to get x_new:
            z = 0;
            for (size_t i = 0; i < npar; i++) {
                x_new[i] = x[i];
                for (size_t j = 0; j <= i; j++) {
                    x_new[i] += lm[z] * dx[j];
                    z++;
                }
            }
            double nll_new = nllikelihood(x_new.get());
            if ((nll_new <= nll) || (rand.uniform() < exp(nll - nll_new))) {
                if(it > burn_in){
                    res.fill(x.get(), nll, weight);
                    weight = 1;
                }
                x.swap(x_new);
                nll = nll_new;
            } else if(it > burn_in){
                weight++;
            }
        }
        res.fill(x.get(), nll, weight);
        res.end();
    }
    
    
    /** \brief estimate the square root (cholesky decomposition) of the covariance matrix of the problem.
     *
     * The method will start a Markov chain at the given startvalues with the \c iterations iterations.
     * The found covariance matrix is used in a next pass where the Markov chain jumping rules
     * are adjusted to the covariance found in the previous step. This procedure is repeated until the
     * estimate is stable.
     *
     * \param rnd is the random number generator to use
     * \param nll is the negative log-posterior to evaluate
     * \param fixed_parameters contains the values of all parameters which shall be considered fixed
     * \param vm is required to estimate good/allowed start values and step sizes for the first pass.
     * \param[out] startvalues will contain the suggested startvalues. The contents when calling this function will be ignored.
     */
    Matrix get_sqrt_cov(Random & rnd, const NLLikelihood & nll, const ParValues & fixed_parameters, const boost::shared_ptr<VarIdManager> vm, std::vector<double> & startvalues);
   
/** \brief Claculate the cholesky decomposition, but allow zero eigenvalues.
 *
 *
 * Writes the cholesky decomposition into \c result. This function treats the case of fixing a parameter through
 * setting its covariance diagonal entry to zero correctly in only decomposing the non-zero part of the matrix
 * and keeping the zero entries where they were.
 *
 * If \c expect_reduced is non-negative, it will be checked whether it matches the determined number
 * of redued (=non-fxed) dimensions. If it does not, an Exception will be thrown. If \c expect_reduced
 * is negative, no check will be done.
 */
void get_cholesky(const Matrix & cov, Matrix & result, int expect_reduced = -1);
    
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


/* \c ipar is the index of the parameter you want the quantile estimate of. */
/*template<class nlltype, class rndtype>
double rafteryLewisQuantile(size_t ipar, double q, double r, double s, const nlltype & nll,
    const std::vector<double> & startvalues, const Matrix & cov, rndtype & rnd);*/

/* some "private" functions used by rafteryLewis.
 * ipar, q, s and r are as above.
 */
/*static void calculate_mn(size_t ipar, double q, double s, double r,
   const FullResultMem & fr, double & m, double & n, double epsilon = -1.0); // epsilon defaults to r/10 

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
   // Simulation 1.: make a run with N_min to determine pilot sample:
   FullResultMem fr(npar, N_min);
   metropolisHastings<nlltype, FullResultMem, rndtype>(nll, fr, rnd, startvalues, cov, N_min, N_min);
   // Simulation 2.: another run for getting the cov right this time:
   Matrix our_cov(fr.getCov());
   fr.reset();
   metropolisHastings<nlltype, FullResultMem, rndtype>(nll, fr, rnd, startvalues, our_cov, N_min, N_min);;
   // Simulation 3.--N.: calculate M and N with the information obtained so far
   // and use those values for the subsequent runs. Continue until we have more
   // iterations thgan suggested (burn-in is discarded in the moment).
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
   // else determine the quantile of the last run and return it:
   return find_quantile(ipar, q, fr);
}
*/

}

#endif
