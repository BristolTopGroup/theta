#ifndef PRODUCER_HPP
#define PRODUCER_HPP

#include "interface/decls.hpp"
#include "interface/plugin.hpp"

#include <vector>
#include <string>
#include <sstream>

#include <boost/ptr_container/ptr_vector.hpp>
#include "interface/plugin.hpp"

namespace theta {

/** \brief The abstract base class for all statistical methods or other objects creating per-event data
 *
 * It is called "producer" as it produces results, given Data and
 * a Model. Every Producer belongs to exactly one Run, which calls its produce method (typically
 * repeatedly on some pseudo data).
 */
class Producer: public theta::plugin::ProductsTableWriter{
public:
    /// Define us as the base_type for derived classes; required for the plugin system
    typedef Producer base_type;
    
    /** Declare the destructor as virtual, as we expect polymorphic
     *  access to derived classes.
     */
    virtual ~Producer(){}
    
    /** \brief Run a statistical algorithm on the data and model and write out the results
     *
     * The result should be written to \c table by calling ProducerTable::set_column on
     * columns previously defined in define_table.
     *
     * Derived classes may assume that all calls are done with the same \c model.
     *
     * In case of an error, the method should through an Exception.
     */
    virtual void produce(Run & run, const Data & data, const Model & model) = 0;
        
protected:
    /** \brief Construct from a Configuration instance
     *
     * Parses the settings "add-nllikelihood-functions".
     */
    Producer(const plugin::Configuration & cfg);
    
    /** \brief Get the likelihood for the provided Data and Model, including the setting of \c override-parameter-distribution, if applicable
     * 
     * Derived classes should always use this method and never construct the NLLIkelihood
     * directly from a Model instance to ensure consistent treatment of the additional likelihood terms.
     */
    std::auto_ptr<NLLikelihood> get_nllikelihood(const Data & data, const Model & model);

    boost::shared_ptr<theta::Distribution> override_parameter_distribution;
};


}

#endif
