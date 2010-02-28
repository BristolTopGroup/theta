#ifndef RESULT_H
#define RESULT_H

#include <algorithm>
#include <vector>
#include "interface/matrix.hpp"
#include "interface/histogram.hpp"

namespace theta {

    /** \brief Class to be used as second argument to metropolisHastings to save some information of the Markov-Chain
     *
     * Also useful as base class for more specialized classes for use with metropolisHastings
     */
    class Result {
    protected:
        /// number of parameters of the likelihood function
        size_t npar;
        
        /// number of total points in the chain (including rejected proposal points)
        size_t count;
        
        /// number of different points in the chain, i.e., not counting rejected proposal points
        size_t count_different_points;
        
        /// sum of the parameter values in each dimension
        std::vector<double> sum;
        
        /// sumsquares(i,j) constains the sum of the chain so far over par_i * par_j
        Matrix sumsquares;
    public:
        /** \brief Construct result with \c npar parameters
         */
        Result(size_t npar);
        
        /// Declare destructor virtual to allow polymorphic access to derived classes
        virtual ~Result();
        
        /// Reset everything to collect data from a new chain
        void reset();
        
        /// Called by metropolisHastings at the end of a chain, here a no-op
        void end();
        
        /** \brief fill a new chain point with the given parameter values, nll value and weight
         *
         * This method is called by the metropolisHastings routine.
         * \param p contains getnpar() parameter values of the point
         * \param nll is the negative logarithm of the likelihood / posterior
         * \param weight is the weight of the point, i.e., the number of rejected proposals to jump away from it, plus one.
         */
        void fill(const double * p, double nll, size_t weight) throw();
        
        /** \brief Combine the common information with another chain
         *
         * Only the mean values and covariance matrix will be combined.
         *
         * If \c res has a different number of parameters, an InvalidArgumentException will be thrown
         */
        void combineResult(const Result & res);
        
        /// Returns the number of parameters specified in the constructor
        size_t getnpar() const;
        
        /// Returns the number of points in the chain, also counting rejected proposal points
        size_t getCount() const;
        
        /// Returns the number of different point in the chain, i.e., not including rejected proposals
        size_t getCountDifferent() const;
        
        /// Returns the mean of the parameter values in the chain
        std::vector<double> getMeans() const;
        
        /// Returns the standard deviation of the marginal distribution of the parameters
        std::vector<double> getSigmas() const;
        
        /// Returns the covariance matrix of the parameter values in the chain
        Matrix getCov() const;
    };

    /*class FullResultMem: public Result{
    private:
        size_t nallocvec;
        double* results;
        size_t* counts;
        inline void ensureCapacity(size_t cap);
    public:
        FullResultMem(size_t npar, size_t cap = 1024);
        ~FullResultMem();
        void fill(const double * p, double nll, size_t weight);
        //i has to be 0 to i<getCountDifferent().   getCountRes(i) is the count (=weight) of the given point, getResult(i) is the point itself. 
        const double* getResult(size_t i) const;
        size_t getCountRes(size_t i) const;
    };

    class HistoResult : public Result {
    private:
        std::vector<Histogram> histos;

    public:
        void fill(const double*, double, size_t);
        //takes limits of the histograms from nll.
        HistoResult(const std::vector<std::pair<double, double> > & ranges, const std::vector<size_t> & nbins);
        ~HistoResult();
        void reset();

        //returns a pointer to the histogram for the i-th parameter.
        const Histogram & getHistogram(size_t i) const;
    };*/

}

#endif
