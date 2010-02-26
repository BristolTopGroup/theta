#ifndef RUN_PLAIN_HPP
#define	RUN_PLAIN_HPP

#include "interface/plugin.hpp"
#include "interface/run.hpp"
#include "interface/plugin_so_interface.hpp"

#include <string>

/** \brief A scan run, scanning over a parameter, creating pseudo data and calling the producers.
 *
 * \todo documentation should go mainly to the Run object. Only specific things here ...
 *
 * The configuration is done via a setting like:
 * <pre>
 * {
 *   //common parameters for Run:
 *   type = "scan_run";
 *   result-file = "result/abc.db";
 *   producers = ("hypotest");
 *   n-events = 10000;
 *   model = "gaussoverflat";
 *
 *   run-id = 2; //optional. Default is 1.
 *   seed = 15; //optional, Default is -1.
 *
 *   //specific to scan_run:
 *   scan-parameter = "beta_signal";
 *   scan-parameter-values = [0.0, 1.0];
 * }
 * </pre>
 *
 * The only parameters specific to this run type are:
 *
 * \c scan-parameter : the parameter name to scan through 
 *
 * \c scan-parameter-values: an array of floating point vales to scan through.
 *
 * Note that each parameter value opens a new runid.
 */
class scan_run: public theta::Run {
public:
    scan_run(theta::plugin::Configuration & ctx);
protected:
    /** \brief Actual run implementation.
     *
     * Run::run delegates the actual run to run_impl, which is overridden here.
     */
    virtual void run_impl();
private:
   std::vector<double> scan_values;
   theta::ParId pid;
};


#endif
