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
    
    /** \brief Run a statistical algorithm on the data and model and write out the results.
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
    
    /** \brief Additional information to add to the info field of a ProducerInfoTable
     *
     * The value returned by this method will be written to the \link ProducerInfoTable
     * ProducerInfoTable \endlink of the current \link Run Run \endlink.
     *
     * Subclasses can override this and define their own semantics for it. Typically, it contains
     * some information about a setting.
     *
     * The default implementation is to return the empty string.
     */
    virtual std::string get_information() const {
        return "";
    }
    
protected:
    /** \brief Construct from a Configuration instance
     *
     * Uses the settings "fix-parameters" and "add-nllikelihood-functions"
     * to fill these objects.
     */
    Producer(const plugin::Configuration & cfg);
    
    /** \brief Get the likelihood for the provided Data and Model, including any additional terms
     */
    NLLikelihood get_nllikelihood(const Data & data, const Model & model);
    
    std::auto_ptr<theta::ParameterSource> fix_parameters;
    boost::ptr_vector<theta::Function> additional_likelihood_terms;
};

}

#endif
