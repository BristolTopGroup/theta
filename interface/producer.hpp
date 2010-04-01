#ifndef PRODUCER_HPP
#define PRODUCER_HPP

#include "interface/decls.hpp"
#include "interface/plugin.hpp"

#include <vector>
#include <string>
#include <sstream>

#include <boost/ptr_container/ptr_vector.hpp>

namespace theta {

/** \brief The abstract base class for all statistical methods or other objects creating per-event data
 *
 * It is called "producer" as it produces results, given Data and
 * a Model. Every Producer belongs to a Run, which calls its produce method (typically
 * often on some pseudo data).
 */
class Producer: public theta::plugin::PluginType, public theta::plugin::EventTableWriter{
public:
    /// Define us as the base_type for derived classes; required for the plugin system
    typedef Producer base_type;
    
    /** Declare the destructor as virtual, as we expect polymorphic
     *  access to derived classes.
     */
    virtual ~Producer(){}
    
    /** \brief Run a statistical algorithm on the data and model and write out the results
     *
     * The method \c produce is called for each pseudo experiment. It should execute
     * the method and write the result to the results database (which is
     * available through the Run object).
     *
     * Derived classes may assume that subsequent calls are done with the same \c model.
     *
     * The result should be written to \c table by calling ProducerTable::set_column on
     * columns previously defined in define_table.
     *
     * In case of an error, the method should through an Exception.
     */
    virtual void produce(Run & run, const Data & data, const Model & model) = 0;
        
protected:
    /** \brief Construct from a Configuration instance
     *
     * Parses the settings "add-nllikelihood-functions" and "override-parameter-distribution".
     */
    Producer(const plugin::Configuration & cfg);
    
    /** \brief Get the likelihood for the provided Data and Model, including any additional terms
     */
    NLLikelihood get_nllikelihood(const Data & data, const Model & model);
    
    std::auto_ptr<Distribution> override_parameter_distribution;
    boost::ptr_vector<theta::Function> additional_likelihood_terms;
};

/** \brief Base class for all statistical methods respecting the strong likelihood principle
 *
 * In contrast to Producer, derived classes have only access to the likelihood function.
 */
class ProducerLikelihoodPrinciple: public Producer{
    public:
        
        /// Implements the produce routine from Producer to call the likelihood version of produce
        virtual void produce(Run & run, const Data & data, const Model & model);
        
        /** \brief Run a statistical algorithm on the data and model and write out the results
         *
         * Equivalent to Producer::produce, but has only access to the likelihood function.
         * Derived classes must implement this.
         */
        virtual void produce(Run & run, const NLLikelihood & nll) = 0;
        
    protected:
        
        ProducerLikelihoodPrinciple(const plugin::Configuration & cfg);
        
        /** \brief Get the likelihood for the provided Model, assuming average data from the model
         *
         * While producers respecting the likelihood principle have only access to
         * the likelihood function in their produce routine, they can make use of an
         * "average" likelihood in order to calibrate step sizes, etc.
         */
        NLLikelihood get_asimov_likelihood(const Model & model);
        
    private:
        //in order to save the model and likelihood references for the asmiov data,
        // use a struct holding these references:
        struct t_ref_datamodel{
            const Data & data;
            const Model & model;
            t_ref_datamodel(const Data & data_, const Model & model_): data(data_), model(model_){
            }
        };
        
        //required by get_asimov_data:
        std::auto_ptr<t_ref_datamodel> ref_datamodel;
        std::auto_ptr<Data> asimov_data;
};

}

#endif
