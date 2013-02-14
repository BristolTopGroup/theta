#ifndef PLUGINS_MCMC_HPP
#define PLUGINS_MCMC_HPP

#include "interface/decls.hpp"
#include "interface/matrix.hpp"

#include <vector>

#include <boost/shared_ptr.hpp>

/** \brief Abstract base class which receives the result from a Markov Chain
 */
class MCMCResult {
public:
    
    /// Returns the number of parameters
    virtual size_t getnpar() const = 0;
    
    /** \brief fill a new chain point with the given parameter values, nll value and weight
        *
        * This method is called by the metropolisHastings routine.
        * \param x contains getnpar() parameter values of the point
        * \param nll is the negative logarithm of the likelihood / posterior
        * \param weight is the weight of the point, i.e., the number of rejected proposals to jump away from it, plus one.
        */
    virtual void fill(const double * x, double nll, size_t n) = 0;
    
    virtual ~MCMCResult();
};


/** \brief MCMCResult class calculating mean and covariance
    * 
    * This class can be either used directly or used as a base class for derived classes
    * which want to provide additional functionality.
    */
class ResultMeanCov: public MCMCResult {
protected:
    // number of parameters of the likelihood function
    size_t npar;
    
    // number of total points in the chain (including rejected proposal points)
    size_t count;
    
    // number of different points in the chain, i.e., not counting rejected proposal points
    size_t count_different_points;
    
    // sliding mean of the parameter values in the chain
    std::vector<double> means;
    
    // sliding covariance times count
    theta::Matrix count_covariance;
    
    virtual void fill2(const double * p, double nll, size_t weight){}
public:
    /** \brief Construct result with \c npar parameters
        */
    explicit ResultMeanCov(size_t npar);
    
    /// Declare destructor virtual to allow polymorphic access to derived classes
    virtual ~ResultMeanCov();
    
    virtual void fill(const double * x, double nll, size_t weight);
    
    virtual size_t getnpar() const;
    
    /// Returns the number of points in the chain, also counting rejected proposal points
    size_t getCount() const;
    
    /// Returns the number of different point in the chain, i.e., not including rejected proposals
    size_t getCountDifferent() const;
    
    /// Returns the mean of the parameter values in the chain
    std::vector<double> getMeans() const;
    
    /// Returns the covariance matrix of the parameter values in the chain
    theta::Matrix getCov() const;
};    
    

struct mcmc_options{
    std::vector<double> startvalues;
    size_t iterations;
    size_t burn_in;
    mcmc_options(const std::vector<double> & startvalues_, size_t it_, size_t bi_): startvalues(startvalues_), iterations(it_), burn_in(bi_){}
};
    
/** \brief Run the metropolis-Hastings Markov-Chain Monte-Carlo algorithm.
 *
 * @param nllikelihood is a function object which must implement double operator()(const double*) const which returns
 *  the negative logarithm of the likelihood (rather: the posterior) to integrate.
 * @param res is the "container" where the Markov Chain will be saved.
 * @param rand is the random number generator to use to compute the next candidate point of the chain.
 * @param opts specify the start values and number of iterations
 * @param sqrt_cov is (apart from a overall factor) the matrix used to transform the uniform-Gaussian
 *   jumping kernel before adding it to the current point. It should be set to the Cholesky decomposition of the
 *   covariance matrix of the likelihood (or an approximation thereof), hence the name "square root of covariance".
 * @param ignore_inf_nll if false (default), it will be considered as error if the nll function evaluates to infinity at
 *    the startvalues given via \c opts. If this is false, sample will be done anyway. In this case, you should
 *    make sure that the result class \c res can handle infinite nll values.
 */
void metropolisHastings(const theta::Function & nllikelihood, MCMCResult &res, theta::Random & rand, const mcmc_options & opts,
                        const theta::Matrix & sqrt_cov, bool ignore_inf_nll = false);



/// Same as metropolisHastings, but instead of multivariate Gaussian, go along the parameter axes always.
void metropolisHastings_ortho(const theta::Function & nllikelihood, MCMCResult &res, theta::Random & rand, const mcmc_options & opts,
                        const theta::ParValues & widths, bool ignore_inf_nll = false);

/** \brief estimate the square root (cholesky decomposition) of the covariance matrix of the likelihood function
 *
 * The method will start a Markov chain at the given startvalues with the \c iterations iterations.
 * The found covariance matrix is used in a next pass where the Markov chain jumping rules
 * are adjusted to the covariance found in the previous step. This procedure is repeated until the
 * estimate is stable.
 *
 * \param rnd is the random number generator to use
 * \param model is the Model to use to get the asimov data from
 * \param[out] startvalues will contain the suggested startvalues. The contents when calling this function will be ignored.
 */
theta::Matrix get_sqrt_cov2(theta::Random & rnd, const theta::Model & model, std::vector<double> & startvalues,
                    const boost::shared_ptr<theta::Distribution> & override_parameter_distribution,
                    const boost::shared_ptr<theta::Function> & additional_nll_term);

/** \brief Calculate the cholesky decomposition, but allow zero eigenvalues.
 *
 *
 * Writes the cholesky decomposition into \c result. This function treats the case of fixing a parameter through
 * setting its covariance diagonal entry to zero correctly in only decomposing the non-zero part of the matrix
 * and keeping the zero entries where they were.
 *
 * If \c expect_reduced is non-negative, it will be checked whether it matches the determined number
 * of reduced (=non-fixed) dimensions. If it does not, an Exception will be thrown. If \c expect_reduced
 * is negative, no check will be done.
 */
void get_cholesky(const theta::Matrix & cov, theta::Matrix & result, int expect_reduced = -1);


#endif
