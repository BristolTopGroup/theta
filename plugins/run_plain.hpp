#ifndef RUN_PLAIN_HPP
#define	RUN_PLAIN_HPP

#include "interface/decls.hpp"
#include "interface/run.hpp"

#include <string>

/** \brief A plain run, creating pseudo data and calling the producers.
 *
 * A plain run is the most simple run object which, for each pseudo experiment,
 * <ol>
 * <li>creates pseudo data (using the pseudodata model)</li>
 * <li>calls all producers, in turn (using the producer model).</li>
 * </ol>
 *
 * For the configuration, refer to the documentation of the theta::Run class.
 * No additional configuration settings are introduced here.
 *
 * The type setting must always be "plain_run".
 */
class plain_run: public theta::Run {
public:
    /// Construct a plain_run instance using a Configuration object
    plain_run(theta::plugin::Configuration & ctx);
protected:
    /** \brief Actual run implementation.
     *
     * RunT::run delegates the actual run to run_impl, which is overridden here.
     */
    virtual void run_impl();
};

#endif
