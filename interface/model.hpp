#ifndef MODEL_HPP
#define MODEL_HPP

#include "interface/decls.hpp"
#include "interface/variables.hpp"
#include "interface/plugin.hpp"
#include "interface/phys.hpp"
#include "interface/histogram-function.hpp"

#include <vector>
#include <string>
#include <set>
#include <map>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_map.hpp>

namespace theta {

    /** \brief Represents a statistical model, i.e., a map  parameters -&gt; observable templates
     *
     * See the \ref model_def "Model definition" in the introduction for a more complete discussion of the
     * model specification.
     * 
     * A Model is used together with Data to construct the likelihood function via Model::getNLLikelihood()
     *
     * A simple example of a complete model specification would be
     *
     * \code
     * mymodel = {
     *    o = { //asuming o was declared an observable
     *       signal = { //component names can be chosen freely
     *          coefficient-function = { / * some  function definition to use as coefficient for the signal template * / };
     *          histogram = "@flat-unit-histo";
     *       };
     *       background = {
     *          coefficient-function = { / * some function definition to use as coefficient for the background template * / };
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
     * //see fixed_poly documentation for details; can also use any other HistogramFunction here.
     * flat-unit-histo = {
     *    type = "fixed_poly";.
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
     *   <li>A \c parameter-distribution setting group which defines the overall parameter distribution of
     *       all model parameters.</li>
     * </ul>
     *
     * Each setting group representing an observable specification contains one or more component specifications.
     * The component names may be chosen freely ("signal" and "background" in the above example).
     * Each component specification is in turn a setting group which contains
     * a histogram specification in a "histogram" setting and a specification of a HistogramFunction in the "coefficient-function" setting.
     *
     */
    class Model {
    public:
        /// define Model as type to use for the plugin system
        typedef Model base_type;
        
        /** \brief Get all parameters the model prediction depends upon
         */
        ParIds getParameters() const;
        
        /** \brief Get all observables the model models a template for
         */
        ObsIds getObservables() const;
        
        /** \brief Creates a likelihood function object for this model, given the data.
         *
         * The observables of this Model and the given Data must be the same. Otherwise,
         * an InvalidArgumentException is thrown.
         *
         * The likelihood will depend on all parameters of this model.
         *
         * The returned object is tightly coupled with this Model and the supplied data object. Ensure that both,
         * this Model's lifetime and the lifetime of \c data, exceed the lifetime of the returns NLLikelihood object.
         */
        virtual std::auto_ptr<NLLikelihood> getNLLikelihood(const Data & data) const = 0;

        /** \brief Fill the prediction for all observables, given parameter values
         *
         *   The returned Histograms in \c data are built as a linear combination of HistogramFunctions using coefficients as previously set
         *   by \c set_prediction. The HistogramFunctions and coefficients are evaluated using the values in \c parameters.
         *
         *  \sa set_prediction
         */
        virtual void get_prediction(Data & result, const ParValues & parameters) const = 0;

        /** \brief Like \c get_prediction, but fluctuate the histograms within their parametrization errors.
         *
         * This function should be used in place of \c get_prediction, if sampling pseudo data from the model.
         * It calls HistogramFunction::getRandomFluctuation instead of HistogramFunction::operator().
         */
        virtual void get_prediction_randomized(Random & rnd, Data & result, const ParValues & parameters) const = 0;

        /** \brief Returns a reference to the parameter distribution
         *
         * The returned Distribution contains (at least) the parameters of this Model.
         *
         * The returned reference is only valid as long as this Model's lifetime.
         */
        virtual const Distribution & get_parameter_distribution() const = 0;
        
        virtual ~Model(){}

    protected:
        boost::shared_ptr<VarIdManager> vm;
        ParIds parameters;
        ObsIds observables;

        explicit Model(const boost::shared_ptr<VarIdManager> & vm);
    };
    
    
    /** \brief The default model in theta
     */
    class default_model: public Model{
    private:
        //The problem of std::map<ObsId, ptr_vector<Function> > is that
        // this requires ptr_vector to be copy-constructible which in turn
        // requires Function to have a new_clone function ...
        //in order to save rewrite, typedef the coeffs and histos map types ...
        typedef boost::ptr_map<ObsId, boost::ptr_vector<HistogramFunction> > histos_type;
        typedef boost::ptr_map<ObsId, boost::ptr_vector<Function> > coeffs_type;
        histos_type histos;
        coeffs_type coeffs;
        std::auto_ptr<Distribution> parameter_distribution;
        
        void set_prediction(const ObsId & obs_id, boost::ptr_vector<Function> & coeffs, boost::ptr_vector<HistogramFunction> & histos);
     public:
     
        default_model(const plugin::Configuration & cfg);
        //the pure virtual functions:
        virtual void get_prediction_randomized(Random & rnd, Data & result, const ParValues & parameters) const;
        virtual void get_prediction(Data & result, const ParValues & parameters) const;
        virtual std::auto_ptr<NLLikelihood> getNLLikelihood(const Data & data) const;
        virtual const Distribution & get_parameter_distribution() const {
           return *parameter_distribution;
        }
        
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
        
    public:
        /** \brief Set the additional term for the negative log-likelihood
         *
         * The result of the function evaluation will add these function values.
         *
         * This can be used as additional constraints / priors for the likelihood function or
         * external information.
         */
        virtual void set_additional_term(const boost::shared_ptr<Function> & term) = 0;
        
        /** \brief Set an alternate prior distribution for the parameters
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
        virtual void set_override_distribution(const boost::shared_ptr<Distribution> & d) = 0;
        
        /** \brief Returns the currently set parameter distribution used in the likelihood evaluation
         * 
         * This is either the distribution from the model, or, if set_override_distribution has been called,
         * the Distribution instance given there.
         * 
         * The returned Distribution reference depends exactly on the model parameters (which is the
         * same as getParameters()).
         * 
         */
        virtual const Distribution & get_parameter_distribution()const = 0;
        
        ///Make destructor virtual as this is an abstract base class
        virtual ~NLLikelihood(){}
    };
    
    
    class default_model_nll: public NLLikelihood{
    friend class default_model;
    public:
        using Function::operator();
        virtual double operator()(const ParValues & values) const;
        
        virtual void set_additional_term(const boost::shared_ptr<Function> & term);
        virtual void set_override_distribution(const boost::shared_ptr<Distribution> & d);
        virtual const Distribution & get_parameter_distribution() const{
            if(override_distribution) return *override_distribution;
            else return model.get_parameter_distribution();
        }
        
    private:
        const default_model & model;
        const Data & data;

        const ObsIds obs_ids;
        
        boost::shared_ptr<Function> additional_term;
        boost::shared_ptr<Distribution> override_distribution;

        std::map<ParId, std::pair<double, double> > ranges;

        //cached predictions:
        mutable Data predictions;
        
        default_model_nll(const default_model & m, const Data & data, const ObsIds & obs);
     };
    
}

#endif
