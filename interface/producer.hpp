#ifndef PRODUCER_HPP
#define PRODUCER_HPP

#include "liblbfgs/lbfgs.h"
#include "libconfig/libconfig.h++"

#include "interface/phys.hpp"
#include "interface/variables.hpp"
#include "interface/database.hpp"
#include "interface/minimizer.hpp"

#include "interface/decls.hpp"

#include <vector>
#include <string>
#include <sstream>

namespace theta {

/** \brief The abstract base class for all statistical methods.
 *
 * It is called "producer" as it produces statistical results, given Data and
 * a Model.
 *
 * To implement your own Producer,
 * <ol>
 * <li>Derive a class from Producer and implement all pure virtual functions</li>
 * <li>Subclass a ProducerFactory and implement all pure virtual methods. This
 *   is where an instance of your new producer class will be built from the
 *   configuration. Make sure to use the REGISTER_FACTORY macro on your ProducerFactory.</li>
 * </ol>
 */
class Producer{
public:
    /** Declare the destructor as virtual, as we expect polymorphic
     *  access to derived classes.
     */
    virtual ~Producer(){}
    
    /** The method \c produce is called for each pseudo experiment. It should execute
     * the method and write the result to the results databse (which is
     * available through the Run object).
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
    std::string getName() const{
        return name;
    }

protected:
    Producer(const std::string & name_): name(name_){}
private:
    std::string name;
};


class MCMCQuantileProducer: public Producer{
public:
   MCMCQuantileProducer(const ParId & p_id, const std::vector<double> & quantiles, database::Database & db, database::LogTable & log);
   virtual void pre_run(const Run & run);
   virtual void produce(int runid, int eventid, const Data & data, const Model & model);
private:
   ParId p_id;
   std::vector<double> quantiles;
   database::MCMCQuantileTable table;
};


}

#endif
