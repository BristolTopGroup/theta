#ifndef PRODUCER_HPP
#define PRODUCER_HPP

#include "libconfig/libconfig.h++"

#include "interface/decls.hpp"
#include "interface/plugin_so_interface.hpp"

#include <vector>
#include <string>
#include <sstream>

namespace theta {

/** \brief The abstract base class for all statistical methods.
 *
 * It is called "producer" as it produces statistical results, given Data and
 * a Model. One important usage is that of Run, which holds a list of producers and calls them
 * on some pseudodata.
 */
class Producer{
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
     * In case of an error, the method should through an Exception.
     * In case of warnings, a log entry should be made calling Run::log_warning.
     */
    virtual void produce(Run & run, const Data & data, const Model & model) = 0;

    /** \brief Returns the name of the producer, as given in the configuration file.
     *
     * The name of the configuration setting group for this producer will be
     * saved in the producer. It is used for logging and database table names,
     * as this name is unique.
     */
    std::string get_name() const{
        return name;
    }
    
    /** \brief Returns the type of the producer, i.e., the C++ class name.
     *
     * This is also what is given as the type="..." setting of the producer configuration.
     */
    std::string get_type() const{
        return type;
    }
    
    /** \brief Additional information to add to the info field of a ProducerInfoTable
     *
     * The value returned by this method will be written to the \link ProducerInfoTable ProducerInfoTable \endlink
     * of the current \link Run Run \endlink.
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
    /// to be called by derived classes in order to fill name and type.
    Producer(const plugin::Configuration & cfg): name(cfg.setting.getName()), type(static_cast<const char*>(cfg.setting["type"])){
    }
private:
    std::string name;
    std::string type;
};

}

#endif
