#include "plugins/mcmc.hpp"
#include "plugins/mcmc-result.hpp"
#include "interface/utils.hpp"

#include <algorithm>
#include <cassert>

#include <limits>

#include <iostream>
#include <cstdio>
#include <sstream>

using namespace std;

namespace theta{

    /*
double find_quantile2(double q, double * list, size_t count){
   size_t index = static_cast<size_t>(q*count + 0.5);
   if(index >= count) index = count-1;
   std::nth_element(list, list+index, list+count);
   return list[index];
}
*/

/* Find the quantile q in the given list specified by list and size count if it is known
 * that the quantile lies in the sublist specified by lower and upper (inclusively!).
 * min and max are the exact minimum and maximum values of the sublist. 
 * The value returned does not have to be an actual element value of the list as linear
 * interpolation is used. */
/*
double find_quantile(double q, double * list, size_t count, int lower, 
   int upper, double min, double max){
    int q_index = static_cast<int>(q*count+0.5);
    if(q_index > upper) q_index = upper;
    if(q_index<lower) q_index = lower;
   while(true){
      assert(lower <= upper);
      if(lower == upper || min==max){
         return list[lower];
      }
      // assert we are in the correct range:
      assert(q_index >= lower);
      assert(q_index <= upper);
      assert(min <= max);
      // Estimate a pivot element by linear regression.
      double pivot = (max - min) / (upper - lower + 1) * (q_index - lower) + min;
      assert(pivot >= min);
      assert(pivot <= max);
      if(upper == lower + 1){
         return pivot;
      }
      // divide the list between lower and upper using pivot:
      int lower_i = lower;
      int upper_i = upper;
      double min_right = max, min_left = max;
      double max_right = min, max_left = min;
      while(lower_i <= upper_i){
         double e;
         while((e = list[lower_i]) <= pivot && lower_i<=upper){
            min_left = std::min(min_left, e);
            max_left = std::max(max_left, e);
            lower_i++;
         }
         while((e = list[upper_i]) > pivot){
            min_right = std::min(min_right, e);
            max_right = std::max(max_right, e);
            upper_i--;
         }
         assert(lower_i <= upper_i+1);
         if(upper_i > lower_i){
            // swap elements:
            std::swap(list[lower_i], list[upper_i]);
            min_right = std::min(min_right, list[upper_i]);
            max_right = std::max(max_right, list[upper_i]);
            min_left = std::min(min_left, list[lower_i]);
            max_left = std::max(max_left, list[lower_i]);
            lower_i++;
            upper_i--;
         }
      }
      if(lower_i != upper_i + 1){
         std::cerr << "lower_i: " << lower_i << " upper_i: " << upper_i << " pivot: " << pivot <<
              " list[lower_i]: " << list[lower_i] << " list[upper_i]" << list[upper_i] << std::endl;
      }
      assert(lower_i == upper_i + 1);
      // the pivot element was >= min, so the left list should be non-empty:
      assert(lower_i > lower);
      / * Termination is guaranteed if the list size becomes smaller by at least 1 
       * at every iteration (call this condition (**)).
       * To guarantee that, handle some special cases.
       *
       *  The list is divided in two parts: lower to lower_i-1 (inclusively), include elements which are <= pivot
       *    from lower_i to upper, where elements are > pivot.
       * Now, lower_i >= lower+1 and lower_i <= upper is guaranteed. * /
      assert(lower_i-1 < upper);
      if(q_index <= lower_i-1){
          upper = lower_i-1;
          max = max_left;
          min = min_left;
      }
      else{
          lower = lower_i;
          max = max_right;
          min = min_right;
      }
   }
}
*/

/*
double find_quantile(size_t ipar, double q, const FullResultMem & fr){
/ * Find the parameter value for the quantile. This can be done by sorting the points 
 * in FullResult by the parameter ipar. However, no full sorting is required: imagine
 * quicksort after the choice of the pivot element and the sorting in the lists
 * of smaller and larger elements: then it is already clear in which of the two sublists
 * the parameter value will lie, so only that one is considered further.
 * Also, the first pivot element can be estimated using mean and sigma of the parameter 
 * in question, if one assumes a gaussian-shaped function.
 * So first, put the values in our own array, already "sorted" by our estimated
 * quantile:
 * /
   size_t count = fr.getCount();
   if(count==0){
       std::cerr << "ERROR in find_quantile: result was empty (count==0)" << std::endl;
       throw Exception("find_quantile: result was empty");
   }
   double * pari = new double[count];
   if(pari==0){
       std::stringstream s;
       s << "find_quantile: could not allocate memory for searching quantile (tried to allocate " << count << " doubles).";
       throw Exception(s.str());
   }
   int lower_next = 0;
   int upper_next = count - 1;
   //initial estimate for the quantile:
   double mean_pari = (fr.getMeans())[ipar];
   double sigma_pari = (fr.getSigmas())[ipar];
   double estimated_parq = mean_pari + sigma_pari * utils::phi_inverse(q);
   //double estimated_parq=-1;
   double min_left = std::numeric_limits<double>::infinity(), max_left = -std::numeric_limits<double>::infinity();
   double min_right = min_left, max_right = max_left;
   for(size_t i = 0; i < fr.getCountDifferent(); i++){
      double res = (fr.getResult(i))[ipar];
      size_t c = fr.getCountRes(i);
      if(res > estimated_parq){
         min_right = std::min(min_right, res);
         max_right = std::max(max_right, res);
         for(size_t j = 0; j<c ; j++){
            pari[upper_next--] = res;
         }
      }
      else{
         min_left = std::min(min_left, res);
         max_left = std::max(max_left, res);
         for(size_t j=0; j<c; j++){
            pari[lower_next++] = res;
         }
      }
   }
   // take care of the case where FullResult-Data is inconsistent:
   if(lower_next != upper_next + 1){
      std::cout << "fr inconsistent: lower_next: " << lower_next << " upper_next: " << upper_next 
                << " count: " << count << " count different: "<< fr.getCountDifferent()<< std::endl;
      throw Exception("find_quantile: fr inconsistent.");
   }
   //std::printf("lower_next: %d, upper_next:%d, count:%u\n", lower_next, upper_next, static_cast<unsigned int>(count));
   double parq;
   if(q*count < lower_next-1){ // search left side:
      parq = find_quantile(q, pari, count, 0, lower_next - 1, min_left, max_left);
   }
   else if(q*count > lower_next){ // search right side:
      parq = find_quantile(q, pari, count, lower_next, count-1, min_right, max_right);
   }
   else{
       return pari[lower_next];
   }
   delete[] pari;
   return parq;
}
*/

/*
double calcBIC(char* z, size_t count, size_t kt){
   //calculation see for example http://support.sas.com/rnd/app/papers/bayesian.pdf
   //section "Raftery-Lewis diagnostics". The variables follow the notation there.
   size_t w[2][2][2];
    double w_hat[2][2][2];
    for (size_t i = 0; i < 2; i++) {
        for (size_t j = 0; j < 2; j++) {
            for (size_t k = 0; k < 2; k++) {
                w_hat[i][j][k] = w[i][j][k] = 0;
            }
        }
    }
    // z_{-2}, z_{-1}, z_{0}
    size_t z_m2, z_m1, z_m0;
   // calculate w_{ijk} :
   z_m2 = 0;
   z_m1 = z[0];
   z_m0 = z[kt];
   for(size_t i=2*kt; i < count; i+=kt){
      z_m2 = z_m1;
      z_m1 = z_m0;
      z_m0 = z[i];
      w[z_m2][z_m1][z_m0]++;
   }
   // calculate \hat w_{ijk}:
   for(size_t i=0; i<2; i++){
      for(size_t j=0; j<2; j++){
         for(size_t k=0; k<2; k++){
            w_hat[i][j][k] = static_cast<double>((w[0][j][k] + w[1][j][k]) * (w[i][j][0] + w[i][j][1]))/
        	    (w[0][j][0] + w[0][j][1] + w[1][j][0] + w[1][j][1]);
         }
      }
   }
   // calculate G^2
   double G2 = 0;
   for(size_t i = 0; i < 2; i++){
      for(size_t j = 0; j < 2; j++){
         for(size_t k = 0; k < 2; k++){
             G2 += w[i][j][k] * log(w[i][j][k] / w_hat[i][j][k]);
         }
      }
   }
   G2 *= 2;
   return G2 - 2 * log(count/kt - 2);
}
*/


/*void calculate_mn(size_t ipar, double q, double s, double r,
   const FullResultMem & fr, double &m, double &n, double epsilon){
   if(epsilon <= 0) epsilon = r / 10;
   double q_par = find_quantile(ipar, q, fr);
   // now we have the quantile, compute the z-chain
   size_t count = fr.getCount();
   char * z = new char[count];
   if(z==0){
       std::stringstream s;
       s << "calculate_mn: could not allocate memory for z. Tried to allocate " << count << " chars.";
       throw Exception(s.str());
   }
   size_t next = 0;
   for(size_t i = 0; i < fr.getCountDifferent(); i++){
      double res = (fr.getResult(i))[ipar];
      size_t c = fr.getCountRes(i);
      if(res <= q_par){
         for(size_t j = 0; j < c; j++){
            z[next++] = 1;
         }
      }
      else{
         for(size_t j = 0; j < c; j++){
            z[next++] = 0;
         }
      }
   }
   assert(next == fr.getCount());
   size_t k;
   bool ok = false;
   for(k = 1; count / k > 3; k++){
      double bic = calcBIC(z, count, k);
      if(bic < 0){
         ok = true;
         break;
      }
   }
   if(!ok){
      delete[] z;
      m = n = std::numeric_limits<double>::quiet_NaN();
      return;
   }
   // compute alpha and beta, the parameters of the thinned chain.
   double alpha=0, beta=0, nalpha=0, nbeta=0;
   for(size_t i = k; i < count; i += k){
      //alpha is the transition prob. from 1 to 0 
      if(z[i-k]==1){
         nalpha++;
         if(z[i]==0) alpha++;
      }
      if(z[i-k]==0){
         nbeta++;
         if(z[i]==1) beta++;
      }
   }
   delete[] z;
   beta /= nbeta;
   alpha /= nalpha;
   if(1 - alpha - beta < 1e-12) m = 0;
   else m = log((alpha + beta) * epsilon / std::max(alpha, beta)) / log(1 - alpha - beta);
   double a = utils::phi_inverse((s+1)/2) / r;
   n = (2 - alpha - beta) * alpha * beta/pow(alpha + beta, 3) * a * a;
   m*=k;
   n*=k;
}
*/

void get_cholesky(const Matrix & cov, Matrix & result, int expect_reduced){
    size_t npar_reduced = cov.getRows();
    size_t npar = npar_reduced;
    result.reset(npar, npar);
    //get the overall "scale" of the matrix by looking at the largest diagonal element:
    double m = fabs(cov(0,0));
    for(size_t i=0; i<cov.getRows(); i++){
        m = max(m, fabs(cov(i,i)));
    }
    bool par_has_zero_cov[npar];
    for (size_t i = 0; i < npar; i++) {
       if((par_has_zero_cov[i] = utils::close_to(cov(i,i), 0, m)))
            npar_reduced--;
    }
    if(npar_reduced==0){
       throw InvalidArgumentException("get_cholesky: number of reduced dimensions is zero (all parameters fixed?)");
    }
    if(expect_reduced > 0 && static_cast<size_t>(expect_reduced)!=npar_reduced){
        throw InvalidArgumentException("get_cholesky: number of reduced dimensions not as expected");
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


Matrix get_sqrt_cov(Random & rnd, const NLLikelihood & nll, const ParValues & fixed_parameters, const boost::shared_ptr<VarIdManager> vm, vector<double> & startvalues){
    const size_t n = nll.getnpar();
    const size_t max_passes = 20;
    const size_t iterations = 8000;
    Matrix sqrt_cov(n, n);
    Matrix cov(n, n);
    startvalues.resize(n);
    //a first estimate of the matrix: use the step width determination of the
    // minimizer:
    ParIds par_ids = nll.getParameters();
    assert(par_ids.size() == n); //should hold by construction of NLLikelihood::getnpar(), but who knows ...
    size_t k=0;
    int n_fixed_parameters = 0;
    for(ParIds::const_iterator it = par_ids.begin(); it!=par_ids.end(); ++it, ++k){
        double width;
        if(fixed_parameters.contains(*it)){
            startvalues[k] = fixed_parameters.get(*it);
            ++n_fixed_parameters;
            width = 0.0;
        }
        else{
            const pair<double, double> & range = vm->get_range(*it);
            const double def = vm->get_default(*it);
            startvalues[k] = def;
            
            //get the start step size according to following rules (i.e., take the first which is true):
            // 1. if the parameter is fixed, use 0
            // 2. take the average of 5% of the interval width and 20% of its default value,
            //    if both these widths are neither 0 nor infinite
            // 3. take the value from 2. which is neither 0 nor infinite
            // 4. use 1.0
            double width0 = fabs(0.2*def);
            double width1 = 0.05*(range.second - range.first);
            bool width0_ok = width0 > 0 && !std::isinf(width0);
            bool width1_ok = width1 > 0 && !std::isinf(width1);
            if(width1==0.0){
                width = 0.0;
                ++n_fixed_parameters;
            }
            else if(width0_ok && width1_ok){
                width = 0.5 * (width0 + width1);
            }
            else if(width0_ok){
                width = width0;
            }
            else if(width1_ok){
                width= width1;
            }
            else{
                width = 1.0;
            }
        }
        cov(k, k) = width*width;
    }
    get_cholesky(cov, sqrt_cov, static_cast<int>(n) - n_fixed_parameters);
    
    vector<double> jump_rates;
    jump_rates.reserve(max_passes);
    Result res(n);
    for (size_t i = 0; i < max_passes; i++) {
        res.reset();
        metropolisHastings(nll, res, rnd, startvalues, sqrt_cov, iterations, iterations/10);
        startvalues = res.getMeans();
        cov = res.getCov();
        get_cholesky(cov, sqrt_cov, static_cast<int>(n) - n_fixed_parameters);
        double previous_jump_rate = jump_rates.size()?jump_rates.back():2.0;
        double jump_rate;
        jump_rates.push_back(jump_rate = static_cast<double>(res.getCountDifferent()) / res.getCount());
        //if jump rate looks reasonable and did not change too much in the last iteration: break:
        if(jump_rate > 0.1 and jump_rate < 0.5 and fabs((previous_jump_rate - jump_rate) / jump_rate) < 0.05) break;
        //TODO: more diagnostics(?) startvalues should not change too much, covariance should not change too much. However,
        // what's a good measure of equality here? Relative difference of eigenvalues and angular distance between the eigenvectors?
    }
    if(jump_rates.size()==max_passes){
        stringstream ss;
        ss << "get_sqrt_cov: covariance estimate did not really converge; jump rates were: ";
        for(size_t i=0; i<max_passes; ++i){
            ss << jump_rates[i] << "; ";
        }
        throw Exception(ss.str());
    }
    return sqrt_cov;
}

}
