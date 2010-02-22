#ifndef HISTOGRAM_FUNCTION_HPP
#define HISTOGRAM_FUNCTION_HPP

#include "interface/variables.hpp"
#include "interface/histogram.hpp"
#include "interface/random.hpp"

#include <vector>
#include <algorithm>
#include <memory>

namespace libconfig{
    class Setting;
}

namespace theta {
    class VarValues;

    /** A Histogram-valued function which depends on zero, one or more parameters.
     *
     * This class is used extensively for model building: a physical model is given by specifying
     * the expected observation in one or more observables and this expectation in turn is specified
     * by histograms which depend on the model parameters. As this can be seen as a histogram-valuesd function,
     * the class is called \c HistogramFunction.
     */
    class HistogramFunction{
    public:
        /** Returns the Histogram as function of values. The reference is only guaranteed
         *  to be valid as long as this HistogramFunction object.
         */
        virtual const Histogram & operator()(const ParValues & values) const = 0;

        /** Returns the Histogram as function of some values, but randomly fluctuated
         *  around its parametrization uncertainty.
         *
         * Note that if a derived class does not overload this function, the
         * default behaviour implemented here is to return an un-fluctuated Histogram
         * (see documentation of the derived class for details of the particular behaviour).
         *
         *  HistogramFunctions are used in theta mainly as building blocks for a model. Here, there play the role
         *  of p.d.f. estimations. Building a p.d.f. estimate always involves some uncertainty:
         *  - for p.d.f.s derived through a fit of some function, the function parameters
         *     have an uncertainty due to the finite statistics of the sample it was derived from
         *  - for p.d.f.s derived simply as histograms from some (simulated or actual) data, the limited
         *    sample size of the data introduces a bin-by-bin statistical uncertainty of the p.d.f. estimation.
         *
         * Note that this uncertainty is one inherent to the way the p.d.f. was modeled. It is different
         * from the treatment of other uncertainties in that it is not written explicitely in the likelihood.
         * Rather, for pseudo data creation, this function will be used instead of \c operator(), effectively
         * integrating over this uncertainty.
         *
         * Note that some p.d.f. uncertainties can be incorporated either here or explicitely through additional
         * parameters in the likelihood: for example a p.d.f. desribes with a gaussian parametrization where
         * the width has some error, this could either be treated here or, alternatively, the width can be
         * included as parameter in the likelihood (possibly with some constraint). Choosing between this possibilities
         * is up to the user specifying the model.
         */
        virtual const Histogram & getRandomFluctuation(AbsRandomProxy & rnd, const ParValues & values) const{
            return operator()(values);
        }

        /** The parameters which this HistogramFunction depends on.
         */
        virtual ParIds getParameters() const = 0;
        
        /** Returns whether a derivative w.r.t. pid could be !=0.
         * 
         * Same as getParameters().contains(pid).
         */
        virtual bool dependsOn(const ParId & pid) const = 0;
        
        /** Returns the gradient of the Histogram w.r.t. \c pid at \c values.
        */
        virtual const Histogram & gradient(const ParValues & values, const ParId & pid) const = 0;
        
        virtual ~HistogramFunction(){}
    };

    /** A simple HistogramFunction which always returns the same Histogram, independent of
     * any parameters. It has no error.
     */
    class ConstantHistogramFunction: public HistogramFunction{
    public:
        /** \brief A \c HistogramFunction which does not depend on any parameters and always returns \c histo.
         *
         * \param histo is the Histogram to return on operator()(...).
         *
         *  \sa HistogramFunction getRandomFluctuation
         */
        ConstantHistogramFunction(const Histogram & histo): h(histo){
            h.set(0,0);
            h.set(h.get_nbins()+1,0);
            grad.reset(h.get_nbins(), h.get_xmin(), h.get_xmax());
        }

        /** Returns the Histogram \c h set at construction time.
         */
        virtual const Histogram & operator()(const ParValues & values) const{
            return h;
        }

        /** Returns the empty parameter set as it does not depend on any parameters.
         */
        virtual ParIds getParameters() const{
            return ParIds();//empty set.
        }

        /** Always returns false, as the Histogram is constant and does not depend
         *  on any parameters.
         */
        virtual bool dependsOn(const ParId &) const{
            return false;
        }

        /** Always returns a zero Histogram. Assuming well-written client code, this
         * should never be called, as this code could check dependsOn(pid) and see
         * that it would be a zero-return anyway and save the time to call this function 
         * (and doing something with the result ...).
         */
        virtual const Histogram & gradient(const ParValues & values, const ParId & pid) const{
            return grad;
        }
        
        virtual ~ConstantHistogramFunction(){}

    private:
        Histogram h;
        Histogram grad;
    };


    /** A simple HistogramFunction which always returns the same Histogram, independent of
     * any parameters. It already includes errors in the form of bin-by-bin poisson errors (which
     * are not correlated).
     */
    class ConstantHistogramFunctionError: public HistogramFunction{
    public:
        /** \brief A \c HistogramFunction which does not depend on any parameters and always returns \c histo.
         *
         * \param histo is the Histogram to return on operator()(...).
         * \param error is a Histogram containing relative errors on the bin entries of histo.
         *
         * If bin j of error contains 0.2, it means that the bin content of bin j of histo is affected
         * by a 20% relative uncertainty.
         *
         * Note that all errors must be >= 0. Otherwise, an InvalidArgumentException
         * will be thrown. Also, the \c error and \c histo Histograms must be
         * compatible, i.e., have the same range and number of bins. If not,
         * an InvalidArgumentException is thrown.
         *
         *  \sa HistogramFunction getRandomFluctuation Histogram::check_compatibility
         */
        ConstantHistogramFunctionError(const Histogram & histo, const Histogram & error): h(histo), err(error){
            h.check_compatibility(error);//throws if not compatible
            h.set(0,0);
            h.set(h.get_nbins()+1,0);
            grad.reset(h.get_nbins(), h.get_xmin(), h.get_xmax());
            fluc.reset(h.get_nbins(), h.get_xmin(), h.get_xmax());
            //check that errors are positive:
            for(size_t i=1; i<=h.get_nbins(); ++i){
                if(error.get(i)<0.0) throw InvalidArgumentException("ConstantHistogramFunctionError: error histogram contains negative entries");
            }
        }

        /** Returns the Histogram \c h set at construction time.
         */
        virtual const Histogram & operator()(const ParValues & values) const{
            return h;
        }

        /** Returns the bin-by-bin fluctuated Histogram.
         *
         * For evey bin j, a random number from a gaussian distribution around 1, truncated at 0, with
         * the width taken from bin j of the error-Histogram is drawn. The contents of
         * the histogram is multiplied by this random number and filled in the result
         * histogram bin j.
         *
         * In particular, all bins are fluctuated statistically independently.
         *
         * Note that for large relative errors, some argue that the truncation at zero is
         * not the "natural" solution as it does not look "nice" at zero. If you ever
         * enter the discussion, you should remember that there is no sensible "&lt; 0" for
         * bin entries, so the density of a truncated gaussian is continous for *everywhere*.
         */
        virtual const Histogram & getRandomFluctuation(AbsRandomProxy & rnd, const ParValues & values) const{
            const size_t nbins = h.get_nbins();
            for(size_t i=1; i<=nbins; ++i){
                double c = fluc.get(i);
                double err_i = err.get(i);
                if(err_i==0.0){
                    fluc.set(i, c);
                }
                else{
                    double factor = -1.0;
                    //factor is gaussian around 1.0 with the current error, truncated at zero:
                    while(factor < 0.0){
                        factor = 1.0 + rnd.gauss(err_i);
                    }
                    fluc.set(i, factor * c);
                }
            }
            return fluc;
        }

        /** Returns the empty parameter set as it does not depend on any parameters.
         */
        virtual ParIds getParameters() const{
            return ParIds();//empty set.
        }

        /** Always returns false, as the HistogramFunction is constant and does not depend
         *  on any parameters.
         */
        virtual bool dependsOn(const ParId &) const{
            return false;
        }

        /** Always returns a zero Histogram.
         */
        virtual const Histogram & gradient(const ParValues & values, const ParId & pid) const{
            return grad;
        }

        virtual ~ConstantHistogramFunctionError(){}

    private:
        Histogram h;
        Histogram err;
        Histogram grad;
        mutable Histogram fluc; // the fluctuated Histogram returned by getFluctuatedHistogram
    };

}

#endif
