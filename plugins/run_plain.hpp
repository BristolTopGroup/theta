#ifndef RUN_PLAIN_HPP
#define	RUN_PLAIN_HPP

#include "interface/plugin.hpp"
#include "interface/plugin_so_interface.hpp"

#include <string>


class PlainRun: public theta::Run {
public:
    PlainRun(long seed, const theta::Model & m_pseudodata, const theta::Model & m_producers,
            const std::string & outfilename, int runid, int n_event) :
        theta::Run(seed, m_pseudodata, m_producers, outfilename, runid, n_event) {
    }
protected:
    /** \brief Actual run implementation.
     *
     * Run::run delegates the actual run to run_impl, which is overridden here.
     */
    virtual void run_impl();
};

/** \brief A plain run, creating pseudo data and calling the producers.
 *
 * A plain run is the most simple run object which, for each pseudo experiment,
 * <ol>
 * <li>creates pseudo data (using the pseudodata model)</li>
 * <li>calls all producers, in turn (using the producer model).</li>
 * </ol>
 *
 * The configuration is done via a setting like:
 * <pre>
 * {
 *   type = "plain";
 *   result-file = "result/abc.db";
 *   producers = ("hypotest");
 *   n-events = 10000;
 *   model = "gaussoverflat";
 *
 *   run-id = 2; //optional. Default is 1.
 *   seed = 15; //optional, Default is -1.
 * }
 * </pre>
 *
 * \c type must always be "plain" to create an instance of \c PlainRun.
 *
 * \c result-file gives the path to the result database file to create
 *
 * \c producers is a list of configuration file paths which contain the
 *    definition of the producers to run.
 *
 * \c n-events is the number of pseudo experiments to run.
 *
 * \c model is the model used for pseudo data creation and which is passed to the producers.
 *    Note that you can provide two different models for these steps if you intead use the settings
 *    \c model-pseudodata and \c model-producers
 *
 * \c run-id is an optional setting and specifies which run-id to write to the result table. This setting
 *    might be removed in future versions of this plugin, as it is not really needed at this point.
 *
 * \c seed is the random number generator seed. It is mainly useful for debugging (i.e., if observing some
 *    strange behaviour only for some seeds). The default setting -1 generates a different seed
 *    each time and should be used as default.
 */
class PlainRunFactory: public theta::plugin::RunFactory {
public:
   virtual std::auto_ptr<theta::Run> build(theta::plugin::ConfigurationContext & ctx) const;
   virtual std::string getTypeName() const{
      return "plain";
   }
   virtual ~PlainRunFactory(){}
};


#endif	/* RUN_PLAIN_HPP */
