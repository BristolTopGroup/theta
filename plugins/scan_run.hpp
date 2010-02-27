#ifndef RUN_PLAIN_HPP
#define	RUN_PLAIN_HPP

#include "interface/plugin.hpp"
#include "interface/run.hpp"
#include "interface/plugin_so_interface.hpp"

#include <string>

/** \brief A run which scans over one parameter, creating pseudo data and calling the producers.
 *
 * You can think of the scan_run as a simple for-loop over one parameter
 * which does (for each fixed parameter value) the same as plain_run.
 *
 * The configuration is done via a setting like:
 * <pre>
 * {
 *   //common parameters for all run types:
 *   type = "scan_run";
 *   result-file = "result/abc.db";
 *   producers = ("hypotest");
 *   n-events = 10000;
 *   model = "gaussoverflat";
 *
 *   //specific to scan_run:
 *   scan-parameter = "beta_signal";
 *   scan-parameter-values = [0.0, 1.0];
 *   scan-parameter-fixed = true;
 * }
 * </pre>
 *
 * For the parameters common to all run types, refer to the documentation of theta::RunT.
 *
 * The only parameters specific to this run type are:
 *
 * \c scan-parameter : the parameter name to scan through 
 *
 * \c scan-parameter-values: an array of floating point vales to scan through.
 *
 * \c scan-parameter-fixed : if true, the scan parameter range will be set to an interval only
 *  containing the current scan point. As consequence, the parameter is fixed for all producers.
 *  If set to false, the parameter will only be fixed for pseudodata generation, but will
 *  not be modified otherwise (i.e., any range and constraint configured apply unchanged when
 *  running the producers).
 *
 * \c n-events is the number of pseudo experiments <em>per scan point</em>.
 *
 * For each scan parameter value, the run-id in the result tables is incremented.
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
   bool scan_parameter_fixed;
};

#endif

