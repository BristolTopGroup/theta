#ifndef RESULT_H
#define RESULT_H

#include <algorithm>
#include <vector>
#include "interface/matrix.hpp"
#include "interface/histogram.hpp"

namespace theta {

    /*
    class Result {
    protected:
        size_t npar;
        size_t count;
        size_t count_different_points;
        std::vector<double> sum;
        Matrix sumsquares;
    public:
        Result(size_t);
        virtual ~Result();
        //resets everything to collect data from a new chain (which is based on the a likelihood of the same dimension)
        void reset();
        //called at the end of a chain:
        void end();
        //fill a new value with the given parameter values, nll value and weight.
        void fill(const double * p, double nll, size_t weight) throw();
        void combineResult(const Result & res);
        size_t getnpar() const;
        size_t getCount() const;
        size_t getCountDifferent() const;
        std::vector<double> getMeans() const;
        std::vector<double> getSigmas() const;
        Matrix getCov() const;
    };

    class FullResultMem: public Result{
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
