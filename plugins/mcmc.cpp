#include "plugins/mcmc.hpp"
#include "plugins/asimov_likelihood_widths.hpp"
#include "interface/utils.hpp"
#include "interface/distribution.hpp"
#include "interface/model.hpp"
#include "interface/random.hpp"

#include <algorithm>
#include <limits>

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <sstream>

using namespace std;
using namespace theta;

namespace{

bool close_to(double a, double b, double scale){
   return fabs(a-b) / scale < 1e-15;
}

}

MCMCResult::~MCMCResult(){}

ResultMeanCov::~ResultMeanCov(){}

ResultMeanCov::ResultMeanCov(size_t n) :
    npar(n), count(0), count_different_points(0), means(n, 0.0), count_covariance(n, n) {
}

void ResultMeanCov::fill(const double * p, double nll, size_t weight){
    if(!std::isfinite(nll)) return;
    //factor for the covariance ...
    double factor = count * 1.0 / (count + 1);
    for(size_t i=1; i<weight; ++i){
        factor += (count*count*1.0) / ((count+i) * (count+i+1));
    }
    for(size_t i=0; i<npar; ++i){
        double diff_i = p[i] - means[i];
        for(size_t j=i; j<npar; ++j){
            double diff_j = p[j] - means[j];
            count_covariance(i,j) += factor * diff_i * diff_j;
        }
        //the old means[i] is now no longer needed, as j>=i ...
        means[i] += weight * 1.0 / (count + weight) * diff_i;
    }
    count += weight;
    count_different_points++;
    fill2(p, nll, weight);
}

size_t ResultMeanCov::getnpar() const {
    return npar;
}

size_t ResultMeanCov::getCount() const {
    return count;
}

size_t ResultMeanCov::getCountDifferent() const {
    return count_different_points;
}

std::vector<double> ResultMeanCov::getMeans() const {
    return means;
}

Matrix ResultMeanCov::getCov() const {
    Matrix result(npar, npar);
    for (size_t i = 0; i < npar; i++) {
        for (size_t j = i; j < npar; j++) {
            result(j, i) = result(i, j) = count_covariance(i,j) / count;
        }
    }
    return result;
}


    
void metropolisHastings(const Function & nllikelihood, MCMCResult &res, Random & rand, const mcmc_options & opts,
                        const Matrix & sqrt_cov, bool ignore_inf_nll) {
    const size_t npar = opts.startvalues.size();
    if(npar != sqrt_cov.get_n_rows() || npar!=sqrt_cov.get_n_cols() || npar!=nllikelihood.get_parameters().size() || npar!=res.getnpar())
        throw std::invalid_argument("metropolisHastings: dimension/size of arguments mismatch");
    size_t npar_reduced = npar;
    for(size_t i=0; i<npar; i++){
        if(sqrt_cov(i,i)==0) --npar_reduced;
    }
    theta_assert(npar_reduced > 0);
    double factor = 2.38 / sqrt(npar_reduced);

    //keep the lower triangle   L    of the sqrt_cov to multiply that with deltaX:
    boost::scoped_array<double> lm(new double[npar * (npar + 1) / 2]);
    size_t z = 0;
    for (size_t i = 0; i < npar; i++) {
        for (size_t j = 0; j <= i; j++) {
            lm[z] = sqrt_cov(i,j) * factor;
            theta_assert(std::isfinite(lm[z]));
            z++;
        }
    }

    //0. initialization:
    boost::scoped_array<double> x(new double[npar]);
    boost::scoped_array<double> x_new(new double[npar]);
    boost::scoped_array<double> dx(new double[npar]);
    
    //set the starting point:
    std::copy(opts.startvalues.begin(), opts.startvalues.end(), &x[0]);
    double nll = nllikelihood(x.get());
    if(!std::isfinite(nll) and not ignore_inf_nll) throw Exception("first nll value was inf");
    // if this exception is thrown, it means that the likelihood function at the start values was inf or NAN.
    // One common reason for this is that the model produces a zero prediction in some bin where there is >0 data which should
    // be avoided by the model (e.g., by filling in some small number in all bins with content zero).

    const size_t iter = opts.burn_in + opts.iterations;
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
        for (size_t i = 0; i < npar; ++i) {
            x_new[i] = x[i];
            for (size_t j = 0; j <= i; ++j) {
                x_new[i] += lm[z] * dx[j];
                z++;
            }
        }
        double nll_new = nllikelihood(x_new.get());
        if ((nll_new <= nll) || (rand.uniform() < exp(nll - nll_new))) {
            if(it > opts.burn_in){
                res.fill(x.get(), nll, weight);
                weight = 1;
            }
            x.swap(x_new);
            nll = nll_new;
        } else if(it > opts.burn_in){
            ++weight;
        }
    }
    res.fill(x.get(), nll, weight);
}


void metropolisHastings_ortho(const theta::Function & nllikelihood, MCMCResult &res, theta::Random & rand, const mcmc_options & opts,
                              const std::vector<double> & widths, bool ignore_inf_nll){
    const size_t npar = opts.startvalues.size();
    theta_assert(npar == res.getnpar());
    theta_assert(npar == widths.size());
    const ParIds & parameters = nllikelihood.get_parameters();
    theta_assert(parameters.size() == npar);

    // TODO: test, optimize
    const double factor = 3;

    //0. initialization:
    boost::scoped_array<double> x(new double[npar]);
    boost::scoped_array<double> x_new(new double[npar]);

    //set the starting point:
    std::copy(opts.startvalues.begin(), opts.startvalues.end(), &x[0]);
    double nll = nllikelihood(x.get());
    if(!std::isfinite(nll) and not ignore_inf_nll) throw Exception("first nll value was inf");
    // if this exception is thrown, it means that the likelihood function at the start values was inf or NAN.
    // One common reason for this is that the model produces a zero prediction in some bin where there is >0 data which should
    // be avoided by the model (e.g., by filling in some small number in all bins with content zero).

    const size_t iter = opts.burn_in + opts.iterations;
    size_t weight = 1;
    for (size_t it = 1; it < iter; it++) {
        // randomly choose a dimension:
        size_t index;
        do{
            index = rand.uniform() * npar;
        }while(widths[index] == 0.0);
        for (size_t i = 0; i < npar; i++) {
            x_new[i] = x[i];
        }
        x_new[index] += rand.gauss(widths[index]) * factor;
        double nll_new = nllikelihood(x_new.get());
        if ((nll_new <= nll) || (rand.uniform() < exp(nll - nll_new))) {
            if(it > opts.burn_in){
                res.fill(x.get(), nll, weight);
                weight = 1;
            }
            x.swap(x_new);
            nll = nll_new;
        } else if(it > opts.burn_in){
            ++weight;
        }
    }
    res.fill(x.get(), nll, weight);
}

void get_cholesky(const Matrix & cov, Matrix & result, int expect_reduced){
    size_t npar_reduced = cov.get_n_rows();
    size_t npar = npar_reduced;
    result.reset(npar, npar);
    //get the overall "scale" of the matrix by looking at the largest diagonal element:
    double m = fabs(cov(0,0));
    for(size_t i=0; i<cov.get_n_rows(); i++){
        m = max(m, fabs(cov(i,i)));
    }
    bool par_has_zero_cov[npar];
    for (size_t i = 0; i < npar; i++) {
       if((par_has_zero_cov[i] = close_to(cov(i,i), 0, m)))
            npar_reduced--;
    }
    if(npar_reduced==0){
        throw invalid_argument("get_cholesky: number of reduced dimensions is zero (all parameters fixed?)");
    }
    if(expect_reduced > 0 && static_cast<size_t>(expect_reduced)!=npar_reduced){
        throw invalid_argument("get_cholesky: number of reduced dimensions not as expected");
    }
    //calculate cholesky decomposition    cov = L L^T    of the reduced covariance matrix:
    Matrix cov_c(npar_reduced, npar_reduced);
    size_t row = 0;
    for (size_t i = 0; i < npar; i++) {
        if(par_has_zero_cov[i])continue;
        size_t col = 0;
        for (size_t j = 0; j < npar; j++) {
            if(par_has_zero_cov[j])continue;
            cov_c(row, col) = cov(i,j);
            col++;
        }
        row++;
    }
    cov_c.cholesky_decomposition();
    //store the result in result, correctly filling the zero vectors ...
    row = 0;
    for(size_t i=0; i<npar; i++){
        if(par_has_zero_cov[i])continue;
        size_t col = 0;
        for (size_t j = 0; j < npar; j++) {
            if(par_has_zero_cov[j])continue;
            result(i,j) = cov_c(row, col);
            col++;
        }
        row++;
    }
}

bool jump_rates_converged(const vector<double> & jump_rates){
    if(jump_rates.size() < 2) return false;
    const size_t n = jump_rates.size();
    double last_rate = jump_rates[n-1];
    double nlast_rate = jump_rates[n-2];
    if(last_rate < 0.05) return false;
    if(last_rate > 0.5) return false;
    //if jump rate looks reasonable and did not change too much in the last iterations: break:
    if(fabs((nlast_rate - last_rate) / max(last_rate, nlast_rate)) < 0.10) return true;
    //TODO: otherwise, it could still have converged well enough, but it is difficult to have a very good
    // criterion here ...
    return false;
}


Matrix get_sqrt_cov2(Random & rnd, const Model & model, std::vector<double> & startvalues,
                    const boost::shared_ptr<theta::Distribution> & override_parameter_distribution,
                    const boost::shared_ptr<theta::Function> & additional_nll_term){
    const size_t max_passes = 50;
    const size_t iterations = 8000;
    ParIds parameters = model.get_parameters();
    if(additional_nll_term){
        parameters.insert_all(additional_nll_term->get_parameters());
    }
    const size_t n = parameters.size();
    Matrix sqrt_cov(n, n);
    Matrix cov(n, n);
    startvalues.resize(n);
    
    ParValues widths = asimov_likelihood_widths(model, override_parameter_distribution, additional_nll_term);
    
    size_t k = 0;
    int n_fixed_parameters = 0;
    
    const Distribution & dist = override_parameter_distribution.get()? *override_parameter_distribution : model.get_parameter_distribution();
    ParValues pv_start;
    dist.mode(pv_start);
    Data asimov_data;
    model.get_prediction(asimov_data, pv_start);
    ParIds fixed_pars;
    for(ParIds::const_iterator it = parameters.begin(); it!=parameters.end(); ++it, ++k){
        double width = widths.get(*it) * 2.38 / sqrt(n);
        startvalues[k] = pv_start.get(*it);
        if(width==0.0){
            ++n_fixed_parameters;
            fixed_pars.insert(*it);
        }
        cov(k, k) = width*width;
    }
    theta_assert(k==n);
    get_cholesky(cov, sqrt_cov, static_cast<int>(n) - n_fixed_parameters);
    
    std::auto_ptr<NLLikelihood> p_nll = model.get_nllikelihood(asimov_data);
    p_nll->set_override_distribution(override_parameter_distribution);
    p_nll->set_additional_term(additional_nll_term);
    NLLikelihood & nll = *p_nll;
    vector<double> jump_rates;
    jump_rates.reserve(max_passes);
    for (size_t i = 0; i < max_passes; i++) {
        ResultMeanCov res(n);
        metropolisHastings(nll, res, rnd, mcmc_options(startvalues, iterations, iterations/10), sqrt_cov);
        startvalues = res.getMeans();
        cov = res.getCov();
        get_cholesky(cov, sqrt_cov, static_cast<int>(n) - n_fixed_parameters);
        jump_rates.push_back(static_cast<double>(res.getCountDifferent()) / res.getCount());
        if(jump_rates_converged(jump_rates)) break;
    }
    if(jump_rates.size()==max_passes){
        cout << "WARNING in get_sqrt_cov: covariance estimate did not really converge; jump rates were: ";
        for(size_t i=0; i<max_passes; ++i){
            cout << jump_rates[i] << "; ";
        }
    }
    return sqrt_cov;
}

