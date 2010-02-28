#ifndef HISTOGRAM_FUNCTION_HPP
#define HISTOGRAM_FUNCTION_HPP

#include "interface/decls.hpp"
#include "interface/variables.hpp"
#include "interface/histogram.hpp"

//#include "interface/random.hpp"

/*#include <vector>
#include <algorithm>
#include <memory>*/

namespace theta {
    class VarValues;

    /** \brief A Histogram-valued function which depends on zero or more parameters.
     *
     * This class is used extensively for model building: a physical model is given by specifying
     * the expected observation in one or more observables and this expectation in turn is specified
     * by histograms which depend on the model parameters. As this can be seen as a histogram-valuesd function,
     * the class is called \c HistogramFunction.
     */
    class HistogramFunction{
    public:
        
        /// Define us as the base_type for derived classes; required for the plugin system
        typedef HistogramFunction base_type;
        
        /** \brief Returns the Histogram for the given parameter values.
         *
         * The returned reference is only guaranteed to be valid as long as this HistogramFunction object.
         */
        virtual const Histogram & operator()(const ParValues & values) const = 0;

        /** \brief Returns the Histogram for the given parameter values, but randomly fluctuated around its parametrization uncertainty.
         *
         * If a derived class does not provide its own implementation of this method, the
         * default behaviour implemented here is to return an un-fluctuated Histogram
         * (see documentation of the derived class for details of the particular behaviour).
         *
         *  HistogramFunctions are used in theta mainly as building blocks for a model. Here, there play the role
         *  of p.d.f. estimations. Building a p.d.f. estimate always involves some uncertainty:
         * <ul>
         *  <li>for p.d.f.s derived through a fit of some function, the function parameters
         *     have an uncertainty due to the finite statistics of the sample it was derived from</li>
         *   <li>for p.d.f.s derived simply as histograms from some (simulated or actual) data, the limited
         *    sample size of the data introduces a bin-by-bin statistical uncertainty of the p.d.f. estimation.</li>
         *  </ul>
         *
         * The uncertainty is inherent to the way the p.d.f. was modeled. It is different
         * from the treatment of other uncertainties in that it is not written explicitely in the likelihood.
         * Rather, for pseudo data creation, this function will be used instead of \c operator(), effectively
         * integrating over this uncertainty.
         *
         * Some p.d.f. uncertainties can be incorporated either here or explicitely through additional
         * parameters in the likelihood: for example a p.d.f. desribes with a gaussian parametrization where
         * the width has some error, this could either be treated here or, alternatively, the width can be
         * included as parameter in the likelihood (possibly with some constraint). Choosing between this possibilities
         * is up to the user specifying the model.
         */
        virtual const Histogram & getRandomFluctuation(Random & rnd, const ParValues & values) const{
            return operator()(values);
        }

        /** \brief Returns the parameters which this HistogramFunction depends on.
         */
        virtual ParIds getParameters() const = 0;
        
        /** \brief Returns whether a derivative w.r.t. pid could be !=0.
         * 
         * Same as getParameters().contains(pid).
         */
        virtual bool dependsOn(const ParId & pid) const = 0;
        
        /** \brief Returns the gradient of the Histogram w.r.t. \c pid at \c values.
        */
        virtual const Histogram & gradient(const ParValues & values, const ParId & pid) const = 0;
        
        /// Declare the destructor virtual as there will be polymorphic access to derived classes
        virtual ~HistogramFunction(){}
    };

    /** \brief A simple HistogramFunction which always returns the same Histogram, independent of any parameters.
     *
     * It does not implement any kind of error, i.e., getRandomFluctuation() returns always the same Histogram.
     */
    class ConstantHistogramFunction: public HistogramFunction{
    public:
        /** \brief A \c HistogramFunction which does not depend on any parameters and always returns \c histo.
         *
         * \param histo is the Histogram to return on operator()(...).
         *
         *  \sa HistogramFunction getRandomFluctuation
         */
        ConstantHistogramFunction(const Histogram & histo){
            set_histo(histo);
        }

        /** \brief Returns the Histogram \c h set at construction time.
         */
        virtual const Histogram & operator()(const ParValues & values) const{
            return h;
        }

        /** \brief Returns the empty parameter set as it does not depend on any parameters.
         */
        virtual ParIds getParameters() const{
            return ParIds();//empty set.
        }

        /** \brief Always returns false, as the Histogram is constant and does not depend on any parameters.
         */
        virtual bool dependsOn(const ParId &) const{
            return false;
        }

        /** \brief Always returns a zero Histogram.
         *
         * As this HistogramFunction does not depend on parameters by definition, the gradient is always zero.
         *
         * Assuming well-written client code, this method should never be called, as any code calling it can check dependsOn() first
         * see that it would be a zero-return anyway.
         */
        virtual const Histogram & gradient(const ParValues & values, const ParId & pid) const{
            return grad;
        }

    protected:
        /** \brief Set the constant Histogram to return
         *
         * This method is meant for derived classes which can use it to set the constant Histogram to
         * be returned by operator()
         */
        void set_histo(const Histogram & h_){
           h = h_;
           h.set(0,0);
           h.set(h.get_nbins()+1,0);
           grad.reset(h.get_nbins(), h.get_xmin(), h.get_xmax());
        }
        /** \brief Default constructor to be used by derived classes
         */
        ConstantHistogramFunction(){}
     private:
        Histogram h;
        Histogram grad;
    };


    /** \brief A constant HistogramFunction, including bin-by-bin fluctuation for pseudodata generation.
     * 
     * Similar to ConstantHistogramFunction, but includes bin-by-bin gaussian errors as uncertainty.
     * are not correlated).
     */
    class ConstantHistogramFunctionError: public HistogramFunction{
    public:
        /** \brief Construct from a Histogram and an error Histogram
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
        ConstantHistogramFunctionError(const Histogram & histo, const Histogram & error){
            set_histos(histo, error);
        }

        /** \brief Returns the Histogram \c h set at construction time.
         */
        virtual const Histogram & operator()(const ParValues & values) const{
            return h;
        }

        /** \brief Returns the bin-by-bin fluctuated Histogram.
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
        virtual const Histogram & getRandomFluctuation(Random & rnd, const ParValues & values) const;

        /** \brief Returns the empty parameter set as it does not depend on any parameters.
         */
        virtual ParIds getParameters() const{
            return ParIds();//empty set.
        }

        /** \brief Always returns false, as this HistogramFunction is constant and does not depend on any parameters.
         */
        virtual bool dependsOn(const ParId &) const{
            return false;
        }

        /** \brief Always returns a zero as this HistogramFunction is constant and does not depend on any parameters.
         */
        virtual const Histogram & gradient(const ParValues & values, const ParId & pid) const{
            return grad;
        }

    protected:
        /** \brief Set the Histogram and the errors to to return
         *
         * This method is meant for derived classes which can use it to set the constant Histogram to
         * be returned by operator() and the error Histogram used by getRandomFlutuation(). As documented
         * in ConstantHistogramFunctionError::ConstantHistogramFunctionError, the \c error Histogram
         * contains bin-by-bin relative errors which are assumed to be independent.
         */        
        void set_histos(const Histogram & histo, const Histogram & error){
            h = histo;
            err = error;
            
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
        /** \brief Default constructor to be used by derived classes
         */
        ConstantHistogramFunctionError(){}
        
    private:
        Histogram h;
        Histogram err;
        Histogram grad;
        mutable Histogram fluc; // the fluctuated Histogram returned by getFluctuatedHistogram
    };

}

#endif
