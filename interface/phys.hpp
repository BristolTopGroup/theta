#ifndef PHYS_HPP
#define PHYS_HPP

#include "interface/decls.hpp"

/*#include "interface/histogram.hpp"
#include "interface/histogram-function.hpp"
#include "interface/exception.hpp"
#include "interface/matrix.hpp"
#include "interface/random.hpp"
#include "interface/metropolis.hpp"
#include "interface/distribution.hpp"*/

#include "interface/variables.hpp"

#include <vector>
#include <string>
#include <limits>
#include <set>
#include <map>
//#include <boost/math/special_functions/fpclassify.hpp>

#include <boost/ptr_container/ptr_vector.hpp>

namespace theta {
    
    /** A real-valued function which depends on some variables. */
    class Function{
    public:
        typedef Function base_type;
        /** \brief Evaluate the function at the given parameter values.
         *
         * @return The function value at \c v.
         */
        virtual double operator()(const ParValues & v) const = 0;

        #if 0
        /** \brief Evaluate the gradient of the function at \c.
         *
         * The result is written to \c grad.
         *
         * Derived classes should override that if either the gradient can be calculated
         * analytically first or if they have a better value for the stepsize.
         *
         * @return The function value at \c v.
         */
        virtual double gradient(const ParValues & v, ParValues & grad) const{
            ParValues v_new(v);
            for(ParIds::const_iterator it=par_ids.begin(); it!=par_ids.end(); ++it){
                v_new = v;
                double epsilon = 1e-8 * fabs(v.get(*it));
                if(epsilon == 0.0)epsilon = 1e-8;
                v_new.set(v.get(*it) + epsilon);
                double f_plus = operator()(v_new);
                v_new.set(v.get(*it) - epsilon);
                double f_minus = operator()(v_new);
                grad.set(*it, (f_plus - f_minus) / (2*epsilon));
            }
            return operator()(v);
        }
        #endif

        ///gradient at \c v w.r.t. \c pid.
        virtual double gradient(const ParValues & v, const ParId & pid) const = 0;

        /** \brief Evaluate the function, using the parameter values given as array of doubles.
         *
         * This does the same as operator()(const ParValues&), however, it takes a pointer
         * to an array of doubles instead.
         *
         * This function is provided to make it easier to provide an interface to other libraries, which
         * do not need to know about the theta-style of variable handling (i.e.,
         * \c ParId, \c ParValue, \c VarIdManager classes).
         *
         * The translation from this "external" array format into the internal (i.e.,
         * \c ParValues) format is done by simultaneously iterating over the
         * ParIds as returned by getParameters() and the given array \c x.
         * (As iteration over a \c ParIds object always yields the same order of parameters,
         * this mapping is well-defined.)
         */
        double operator()(const double * x) const{
            size_t i=0;
            ParValues pv;
            for(ParIds::const_iterator it=par_ids.begin(); it!=par_ids.end(); ++it, ++i){
                assert(!isnan(x[i]));
                pv.set(*it, x[i]);
            }
            return operator()(pv);
        }


        /** @return the parameters this function depends on.
         */
        ParIds getParameters() const{
            return par_ids;
        }

        /** \brief The number of parameters this function depends on.
         *
         * Same as \c getParameters().size().
         */
        size_t getnpar() const{
            return par_ids.size();
        }

        virtual ~Function(){}
    protected:
        ParIds par_ids;
        Function(const ParIds & pids): par_ids(pids){}
    };
    
    /** A function which multiplies a list of variables.*/
    class MultFunction: public Function{
    public:
        virtual double operator()(const ParValues & v) const;
        MultFunction(const ParIds & v_ids);
        virtual ~MultFunction(){}
        virtual double gradient(const ParValues & v, const ParId & pid) const;
    };
    
    
    /** An object representing (binned) data for one or more observables.
     *  
     * A data object can be constructed:
     * -# "by hand": use the default constructor and set data Histograms for a number of observables
     * -# by using the DataFactory, which parses a configuration setting and typically reads data from root files
     * -# by sampling from a \c Model using the \c samplePseudoData method.
     *
     * After construction, the Data object is typically used to get a likelihood function from a Model.
     *
     * \sa Model::getLikelihood(Data) DataFactory Model::samplePseudoData
     */
    class Data {
    public:
        /** \brief Returns all obs_ids for which any data was added using addData(obs_id).
         */
        ObsIds getObservables() const;

        /** \brief Add the data for an observables
         *
         * The Histogram dat as observed data for variable obs_id to this container.
         * The Histogram is copied.
         *
         * \param obs_id The observable the data was recorded for.
         * \param dat The Histogram holding data information for that observable.
         */
        void addData(const ObsId & obs_id, const Histogram & dat);

        /** \brief returns the Histogram for the specified observable previously set by addData.
         *
         * If no Histogram was set for this obs_id, a NotFoundException is thrown.
         */
        const Histogram & getData(const ObsId & obs_id) const;

    private:
        std::map<ObsId, Histogram> data;
    };
    
    class Model {
        friend class NLLikelihood;
        friend class ModelFactory;
    public:
       /** \brief Create a new Model with the VarIdManager \c vm.
        */
        Model(boost::shared_ptr<VarIdManager> & vm);
        
        /** \brief Get all parameters the model prediction depends upon
         */
        ParIds getParameters() const;
        
        /** \brief Get all observables the model predicts a distribution for
         */
        ObsIds getObservables() const;
        
        /** \brief Sample pseudo data for the prediction of this Model for model parameters values
         *
         */
        Data samplePseudoData(Random & rnd, const ParValues & values) const;
        
        /** \brief Sample values from the prior Distributions.
         * 
         * Parameters, for which no prior was specified are set to their default values. The returned
         * \c ParValues object contains values for *all* parameters of this model.
         */
        ParValues sampleValues(Random & rnd) const;

        /** \brief Creates a likelihood function object for this model, given the data.
         *
         * The observables of this Model and the given Data must be the same. Otherwise,
         * an InvalidArgumentException is thrown.
         *
         * The likelihood will depend on all parameters of this model.
         *
         * Before calling this method, make sure that everything is set up, i.e.
         * you have to call setPrediction() for every observable specified in the constructor.
         * Otherwise, an InvalidArgumentException will be thrown.
         *
         * The returned object is tightly coupled with this Model and the data. Ensure that both,
         * this Model's lifetime and the lifetime of data, exceed the lifetime of the returns NLLikelihood object.
         */
        NLLikelihood getNLLikelihood(const Data & data) const;

        /* Creates a NLLikelihood function object for this Model, given Data data, but includes only the specified
         *  observables obs_ids and parameters pars. obs_ids and pars must be subset of this model's parameters
         *  and observables, respectively. obs_ids must also be subset of the data's observables.
         *  If one of those conditions is not met, an InvalidArgumentException is thrown.
         *
         * Make sure that both, this Model's lifetime as well as the lifetime of data, exceed the lifetime of the returned NLLikelihood
         * object.
         *
        NLLikelihood getNLLikelihood(const Data & data, const ParIds & pars, const ObsIds & obs_ids) const;
        */

        /** \brief Specify the prediction of an observable
         *
         * Set the prediction of observable \c obs_id to a linear combination of \c histos with \c coeffs as coefficients.
         *  \c coeffs , \c histos and \c component_names must be of same length and non-empty, otherwise, an 
         *   \c InvalidArgumentException is thrown.
         *
         * The objects in \c histos and \c coeffs will be transferred, not copied, i.e. histos.empty() and coeffs.empty() will hold
         * after this function returns.
         */
        void set_prediction(const ObsId & obs_id, boost::ptr_vector<Function> & coeffs, boost::ptr_vector<HistogramFunction> & histos, const std::vector<std::string> & component_names);

        /** Returns the prediction for the observable \c obs_id using the variable values \c parameters into \c result.
        *   The returned Histogram is built as a linear combination of HistogramFunctions using coefficients as previously set
        *   by \c set_prediction. The HistogramFunctions and coefficients are evaluated using the values in \c parameters.
        *
        *  \sa set_prediction
        */
        void get_prediction(Histogram & result, const ParValues & parameters, const ObsId & obs_id) const;

        /** Like \c get_prediction, but randomize the histograms with their parametrization errors.
         *
         * This function should be used in place of \c get_prediction, if sampling pseudo data from the model.
         */
        void get_prediction_randomized(Random & rnd, Histogram &result, const ParValues & parameters, const ObsId & obs_id) const;
        
        /** The derivative of getPredition w.r.t. \c pid. Make sure to copy the returned Histogram as the function
         * returns a reference to intrenal data which is overwritten in the next call to \c getPredictionDerivative
        */
        void get_prediction_derivative(Histogram & result, const ParValues & parameters, const ObsId & obs_id, const ParId & pid) const;

        /** get a single component of the prediction for a given observable. Will return
         * coefficient i * histogram function i
         * as set with setPrediction. throws OutOfBoundsException if i is too large.
         */
        Histogram getPredictionComponent(const ParValues & parameters, const ObsId & obs_id, size_t i) const;

        /** Returns the name of component \c i as set before with \c setPrediction.
        */
        std::string getPredictionComponentName(const ObsId &, size_t i) const;

        /** Returns the number of components for observable \c obs_id as set before with \c setPrediction.
        */
        size_t getPredictionNComponents(const ObsId & obs_id) const;

        /** \brief Adds the given Distribution to the list of priors.
         * 
         * This Model takes the ownership of \c d, i.e., d.get()==0 holds after this function returns.
         * 
         * \todo This method should throw an InvalidArgumentException if a parameter would have two priors
         * \param d The Distribution to add.
         */
        void addPrior(std::auto_ptr<Distribution> & d);

        /** Returns a reference to the prior Distribution previously added with addPrior.
         * 
         * The returned reference is only valid as long as this Model's lifetime.
         * 
         * \param i the index of the distribution; 0 \<= \c i \< getNPriors().
         */
        const Distribution & getPrior(size_t i) const;

        /** Retuns the number of priors added with addPrior.
         */
        size_t getNPriors() const;

    private:
        boost::shared_ptr<VarIdManager> vm;
        ParIds parameters;
        ObsIds observables;
        //the shared_ptr is required as the Data part of a std::map<Key, Data>
        //needs to be copy-constructible.
        //However, for ptr_vector to be copy-contructible, we need new_clone function to
        // implement cloning for HistogramFunctions and Functions. As this
        // requires more implementation work by plugin writers, let's do it a bit more complicated
        // here to save them the work ...
        //shared_ptr should be save, as it has the usual copy semantics (unlike auto_ptr).
        std::map<ObsId, boost::shared_ptr<boost::ptr_vector<HistogramFunction> > > histos;
        std::map<ObsId, boost::shared_ptr<boost::ptr_vector<Function> > > coeffs;
        //in order to save rewrite, typedef the coeefs and histos map types ...
        typedef std::map<ObsId, boost::shared_ptr<boost::ptr_vector<Function> > > coeffs_type;
        typedef std::map<ObsId, boost::shared_ptr<boost::ptr_vector<HistogramFunction> > > histos_type;
        std::map<ObsId, std::vector<std::string> > names;

        //boost::ptr_vector<Distribution> priors;
        std::vector<boost::shared_ptr<Distribution> > priors;
    };
    
    /** \brief Factory class to build Models from configuration settings. 
     */
    class ModelFactory{
    public:
        /** \brief Build a Model from the Setting \c s.
         * 
         * Throws various exceptions in case of errors. Possible exceptins thrown are:
         * <ul>
         * <li> SettingNotFoundException </li>
         * <li> SettingTypeException </li>
         * <li> SettingException </li>
         * <li> Exception </li>
         * 
         * The returned Model will use the VarIdManager given in \c vm. Existing variables
         * (=same name, same specification) will be re-used. Otheriwse, they will
         * be added to \c vm.
         * 
         * \param s The Setting to use to build the Model from.
         * \param vm The VarIdManager which should be used for the Model.
         */
        static std::auto_ptr<Model> buildModel(plugin::Configuration & ctx);
    };    

    /** Function object of a negative log likelihood of a model, given data.
     *
     * An instance cannot be constructed directly; use Model::getNLLikelihood instead.
     *
     * The underflow and overflow bin of the Histograms in the Model are <b>not</b> included in the
     * likelihood. This behaviour is expected by most users, e.g., if you specify a Histogram to have only one
     * bin, the likelihood function should perfectly behave like a counting experiment. If you want to include these
     * bins, add them by hand, i.e. make a histogram which is larger by two bins and set the entries
     * of the first and last bin accordingly.
     *
     * \sa Model::getNLLikelihood
     */
    class NLLikelihood: public Function {
    friend class Model;
    public:
        using Function::operator();

        /** \brief Evaluate the likelihood function, using the parameter values given in \c values.
         * 
         * \param values The parameter values to use for evaluation.
         * \return The value of the negative log likelihood at \c values.
         */
        double operator()(const ParValues & values) const;
        
       /** Calculate the gradient and function value at \c x and fill the result into \c g and \c nll, respectively.
        *
        *  \c g must be allocated by the caller (and is owned by the caller).
        */
        void gradient(const double* x, double* g, double & nll) const;
        
        void gradient2(const double* x, double* g, double & nll) const;

        virtual double gradient(const ParValues & v, const ParId & pid) const;
                
        /** \brief The set of observables that enter the likelihood calculation.
         * 
         * The returned reference is only valid as long as this object exists.
        */
        const ObsIds & getObservables() const{ return obs_ids;}
        
        /** \brief The model which provides the prediction used for calculating the likelihood.
         * 
         * The returned reference is only valid as long as this object exists.
        */
        const Model & getModel() const{return model;}

    private:
        boost::shared_ptr<VarIdManager> vm;
        const Model & model;
        const Data & data;

        const ObsIds obs_ids;

        //values used internally if called with the double* functions.
        mutable ParValues values;
        //the derivatives of the priors:
        mutable ParValues p_derivatives;
        //cached predictions:
        mutable std::map<ObsId, Histogram> predictions;
        mutable std::map<ObsId, Histogram> predictions_d;
        
        NLLikelihood(const boost::shared_ptr<VarIdManager> & vm, const Model & m, const Data & data, const ObsIds & obs, const ParIds & pars);
    };
    
}

#endif	/* _PHYS_HPP */
