#ifndef PHYS_HPP
#define PHYS_HPP

#include "interface/decls.hpp"
#include "interface/variables.hpp"
#include "interface/plugin.hpp"

#include "interface/histogram-function.hpp"

#include <vector>
#include <string>
#include <limits>
#include <set>
#include <map>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_map.hpp>

namespace theta {
    
    /** A real-valued function which depends on one or more parameters
     *
     * This is the base class for function plugins.
     */
    class Function: public theta::plugin::PluginType{
    public:
        /// Define this as the base_type for derived classes; required for the plugin system
        typedef Function base_type;
        
        /** \brief Evaluate the function at the given parameter values.
         *
         * @return The function value at \c v.
         */
        virtual double operator()(const ParValues & v) const = 0;

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
                assert(!std::isnan(x[i]));
                pv.set(*it, x[i]);
            }
            return operator()(pv);
        }


        /** \brief Returns the parameters this function depends on
         */
        const ParIds & getParameters() const{
            return par_ids;
        }

        /** \brief The number of parameters this function depends on
         *
         * Same as \c getParameters().size().
         */
        size_t getnpar() const{
            return par_ids.size();
        }

        /// Declare destructor virtual as polymorphic access to derived classes will happen.
        virtual ~Function(){}
    protected:
        /** \brief The parameters this function depends on
         *
         * Has to be set correctly by derived classes
         */
        ParIds par_ids;
        
        /// Proxy to PluginType constructor expecting a Configuration instance
        Function(const plugin::Configuration & cfg): PluginType(cfg){}
        
        /** \brief Default constructor to allow Function construction without Configuration
         *
         * This is useful for NLLikelihood, as this object is not constructued via the plugin system.
         */
        Function(){}
    };    
    
    /** \brief Contains data for one or more observables
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
    
    /** \brief A data-providing class, can be used as base class in the plugin system
     *
     * DataSource classes are used as part of a run, which, for each pseuso
     * experiment, calls the DataSource::fill function to get the pseudo data.
     */
    class DataSource: public theta::plugin::PluginType, public theta::plugin::EventTableWriter{
    public:
        
        /// Define this as the base_type for derived classes; required for the plugin system
        typedef DataSource base_type;
        
        /** \brief Exception class to be thrown by fill
         */
        class DataUnavailable{};
        
        /** \brief Returns the observables this DataSource provides data for
         *
         * This stays the same over the lifetime of the instance.
         *
         * The return value is the same as \c obs_ids in
         * \code
         * Data data;
         * this->fill(data);
         * ParIds obs_ids = values.getObservables();
         * \endcode 
         */
        virtual ObsIds getObservables() const = 0;
        
        /** \brief Fill the provided Data instance with data
         *
         * This method is called exactly once per event. It sets
         * the histograms of the provided observables and lets everything else unaffected.
         *
         * It can throw a DataUnavailable exception if no more data is available and the
         * run should end.
         */
        virtual void fill(Data & dat) = 0;
        
        /// Declare destructor virtual as polymorphic access to derived classes will happen.
        virtual ~DataSource(){}
        
    protected:
        DataSource(const theta::plugin::Configuration & cfg): theta::plugin::PluginType(cfg){}
    };
    
    /** \brief A parameter-providing class; can be used as base class in the plugin system
     *
     * A typical use is of this class is
     * <ul>
     *  <li>As part of a DataSource, to provide some alternative parameter values to be
     *     used for pseudo-data creation</li>
     *  <li>As part of a producer to fix some parameters.</li>
     * </ul>
     *
     * While it could be useful for some applications,
     * this class does not on itself write to an EventTable, as in many use cases,
     * the same ParameterSource setting is used multiple times with the same name within
     * the same run.
     */
    /*class ParameterSource: public theta::plugin::PluginType{
    public:
        /// Define this as the base_type for derived classes; required for the plugin system
        typedef ParameterSource base_type;
        
        /** \brief Exception class to be thrown by fill
         * /
        class ParametersUnavailable{};
        
        /** \brief Return the parameters this source provides
         *
         * This stays the same over the lifetime of the instance.
         *
         * The return value is the same as \c par_ids in
         *\code
         * ParValues values;
         * this->fill(values);
         * ParIds par_ids = values.getParameters();
         *\endcode
         * /
        virtual ParIds getParameters() const = 0;
        
        /** \brief Fill the provided values instance
         *
         * Sets the configured parameters and leaves everything else unaffected.
         *
         * Can throw a ParametersUnavailable exception, if no (more) parameter
         * values are available.
         * /
        virtual void fill(ParValues & values) = 0;
        
        /// Declare destructor virtual, as there will be polymorphic access
        virtual ~ParameterSource(){}
        
    protected:
        /// Proxy to the constructor of PluginType
        ParameterSource(const theta::plugin::Configuration & cfg): theta::plugin::PluginType(cfg){}
    };*/
    
    /** \brief Provides a mapping from parameters to distributions for one or more observables
     *
     * A Model is used together with Data to construct the likelihood function.
     *
     * For configuration syntax, see ModelFactory.
     */
    class Model {
        friend class NLLikelihood;
        friend class ModelFactory;
    public:
       /** \brief Create a new Model with the VarIdManager \c vm.
        */
        Model(const boost::shared_ptr<VarIdManager> & vm);
        
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
        
        /* \brief Sample values from the prior Distributions.
         * 
         * Parameters, for which no prior was specified are set to their default values. The returned
         * \c ParValues object contains values for *all* parameters of this model.
         */
        //ParValues sampleValues(Random & rnd) const;

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

        /** \brief Returns the prediction for the observable \c obs_id using the variable values \c parameters into \c result.
        *
        *   The returned Histogram is built as a linear combination of HistogramFunctions using coefficients as previously set
        *   by \c set_prediction. The HistogramFunctions and coefficients are evaluated using the values in \c parameters.
        *
        *  \sa set_prediction
        */
        void get_prediction(Histogram & result, const ParValues & parameters, const ObsId & obs_id) const;

        /** \brief Like \c get_prediction, but fluctuate the histograms within their parametrization errors.
         *
         * This function should be used in place of \c get_prediction, if sampling pseudo data from the model.
         */
        void get_prediction_randomized(Random & rnd, Histogram &result, const ParValues & parameters, const ObsId & obs_id) const;

        /** \brief get a single component of the prediction for a given observable.
         *
         * Will return coefficient i * histogram function i
         * as set with setPrediction. throws NotFoundException if i is too large and an InvalidArgumentException
         * if no prediction was previously set for obs_id.
         */
        Histogram getPredictionComponent(const ParValues & parameters, const ObsId & obs_id, size_t i) const;

        /** \brief Returns the name of component \c i as set before with set_prediction.
        */
        std::string getPredictionComponentName(const ObsId &, size_t i) const;

        /** \brief Returns the number of components for observable \c obs_id as set before with set_prediction.
        */
        size_t getPredictionNComponents(const ObsId & obs_id) const;

        /** \brief Returns a reference to the parameter distribution
         * 
         * The returned Distribution contains (at least) the parameters of this Model.
         *
         * The returned reference is only valid as long as this Model's lifetime.
         */
        const Distribution & get_parameter_distribution() const{
            return *parameter_distribution;
        }

    private:
        boost::shared_ptr<VarIdManager> vm;
        ParIds parameters;
        ObsIds observables;

        //The problem of std::map<ObsId, ptr_vector<Function> > is that
        // this requires ptr_vector to be copy-constructible which in turn
        // requires Function to have a new_clone function ...
        //in order to save rewrite, typedef the coeffs and histos map types ...
        typedef boost::ptr_map<ObsId, boost::ptr_vector<Function> > coeffs_type;
        typedef boost::ptr_map<ObsId, boost::ptr_vector<HistogramFunction> > histos_type;
        histos_type histos;
        coeffs_type coeffs;

        std::map<ObsId, std::vector<std::string> > names;
        std::auto_ptr<Distribution> parameter_distribution;
    };
    
    //delete_clone is required by the destructor of boost::ptr_vector.
    //The default implementations does not seem to work on pure abstract
    // classes, which is a real shame.
    namespace{
        inline void delete_clone(const HistogramFunction * r){
            delete r;
        }

        inline void delete_clone(const Function * r){
            delete r;
        }
    }


    /** \brief Factory class to build Models from a configuration settings group
     *
     * See the \ref model_def "Model definition" in the introduction for a more complete discussion of the
     * model specification.
     *
     * A simple example of a complete model specification would be
     *
     * \code
     * mymodel = {
     *    o = { //asuming o was declared an observable
     *       signal = { //component names can be chosen freely
     *          coefficients = ("Theta"); // assuming Theta was declared as parameter
     *          histogram = "@flat-unit-histo";
     *       };
     *       background = {
     *          coefficients = ("mu"); // assuming mu was declared as parameter
     *          histogram = "@flat-unit-histo";
     *       };
     *    };
     *
     *    parameter-distribution = {
     *        type = "product_distribution";
     *        distributions = ("@d1", "@d2");
     *    };
     * };
     *
     * //see fixed_poly documentation for details
     * flat-unit-histo = {
     *    type = "fixed_poly"; 
     *    observable = "o";
     *    normalize_to = 1.0;
     *    coefficients = [1.0];
     * };
     *
     * d1 = {...}; // some distribution
     * d2 = {...}; // some other distribution
     * \endcode
     *
     * The configuration group defining a model contains:
     * <ul>
     *   <li>Exactly one settings group per observable to be modeled by this Model. It has to have the same name
     *      as the observable to be modeled and contains the observable specification.</li>
     *    <li>
     *   <li>A "parameter-distribution" setting group which defines the overall parameter distribution of
     *       (at least) all model parameters.</li>
     * </ul>
     *
     * Each setting group representing an observable specification contains one or more component specifications.
     * The component names may be chosen freely ("signal" and "background" in the above example).
     * Each component specification is in turn a setting group which contains
     * a histogram specification in a "histogram" setting and a list of parameter names in the "coefficients" list.
     *
     * Instead of the "coefficients" setting, a setting group "coefficient-function" can be specified which is
     * used to construct a Function object via the plugin system.
     */
    class ModelFactory{
    public:
        /** \brief Build a Model from a Configuration
         * 
         * The returned Model will use the VarIdManager given in \c ctx.vm
         * 
         * \param ctx The Configuration to use to build the Model.
         */
        static std::auto_ptr<Model> buildModel(const plugin::Configuration & ctx);
    };    

    /** \brief Function object of a negative log likelihood of a model, given data.
     *
     * An instance cannot be constructed directly; use Model::getNLLikelihood instead.
     *
     * The underflow and overflow bin of the Histograms in the Model are <b>not</b> included in the
     * likelihood. This behavior is expected by most users, e.g., if you specify a Histogram to have only one
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
        
        /** \brief The set of observables that enter the likelihood calculation.
         * 
         * The returned reference is only valid as long as this object exists.
        */
        //const ObsIds & getObservables() const{ return obs_ids;}
        
        /** \brief The model which provides the prediction used for calculating the likelihood.
         * 
         * The returned reference is only valid as long as this object exists.
         */
        //const Model & getModel() const{return model;}
        
        /** \brief Set the additional terms for the likelihood
         *
         * The result of the function evaluation will add these function values.
         *
         * This can be used as additional constraints / priors for the likelihood function.
         */
        void set_additional_terms(const boost::ptr_vector<Function> * terms);
        
        /** \brief Set an alternate prior distribution for the parameters
         *
         * The ownership of the memory pointed to by d remains at the caller.
         *
         * In the calculation of the likelihood function, the prior distributions
         * for the parameters are added. By default, the prior distributions from
         * the model are used. However, it is possible to specify an alternative Distribution
         * by calling this method which will be used instead.
         *
         * Calling this function with d=0 means to use the Distribution from the model.
         *
         * \c d must be defined exactly for the model parameters. Otherwise, an InvalidArgumentException
         * will be thrown.
         */
        void set_override_distribution(const Distribution * d);

    private:
        const Model & model;
        const Data & data;

        const ObsIds obs_ids;
        
        const boost::ptr_vector<Function> * additional_terms;
        const Distribution * override_distribution;

        std::map<ParId, std::pair<double, double> > ranges;

        //values used internally if called with the double* functions.
        mutable ParValues values;
        //cached predictions:
        mutable std::map<ObsId, Histogram> predictions;
        mutable std::map<ObsId, Histogram> predictions_d;
        
        NLLikelihood(const Model & m, const Data & data, const ObsIds & obs);
    };
    
}

#endif
