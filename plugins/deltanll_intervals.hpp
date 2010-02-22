#ifndef PLUGIN_DELTANLL_INTERVALS_HPP
#define PLUGIN_DELTANLL_INTERVALS_HPP

#include "interface/plugin_so_interface.hpp"

#include "interface/variables.hpp"
#include "interface/database.hpp"

#include <string>


/** \brief Table to store results of a DeltaNLLIntervalProducer.
 *
 * This table object is used by an instance of \link DeltaNLLIntervalProducer DeltaNLLIntervalProducer \endlink.
 *
 * The corresponding SQL table has following fields:
 * <ol>
 *  <li>\c runid \c INTEGER(4): the run id the entry refers to</li>
 *  <li>\c eventid \c INTEGER(4): the event id the entry refers to</li>
 *  <li>\c clevel \c DOUBLE: confidence level for the lower and upper entries</li>
 *  <li>\c lower \c DOUBLE: the lower limit on the parameter</li>
 *  <li>\c upper \c DOUBLE: the upper limit on the parameter</li>
 * </ol>
 *
 * Note that each DeltaNllIntervalProducer makes one entry per pseudo experiment and
 * configured confidence level (plus always an entry for the zero confidence level).
 */
class DeltaNllIntervalTable: public database::Table {
public:
    DeltaNllIntervalTable(const std::string & name_): database::Table(name_){}

    /** \brief append an entry to the table
     *
     * \param run the \link theta::Run Run \endlink object; used to extract the runid and eventid columns
     * \param clevel the confidence level for the interval
     * \param lower the lower end of the interval
     * \param upper the upper end of the interval
     */
    void append(const theta::Run & run, double clevel, double lower, double upper);
private:
    /** \brief Implementation of Table::create_table to do the actual table creation.
     */
    virtual void create_table();
};

/** \brief Confidence intervals based on the difference in the negative log-likelihood.
 *
 * This producer uses the likelihood to derive confidence intervals based on the
 * based on the asymptotic property of the likelihood ratio test statistics to be distributed
 * according to a chi^2-distribution.
 *
 * Given a likelihood function L which depends on parameters p_0 ... p_n, for which a
 * confidence interval for p0 should be constructed, the method works as follows:
 * <ol>
 * <li>vary parameters p_0 ... p_n to find the maximum value of the likelihood function, L_max</li>
 * <li>scan through the parameter p0. For each new value of po, maximize L with respect to all other
 *     parameters, yielding the maximum as function of p_0, L_m(p_0).</li>
 * <li>The interval for p0 is given by the points where the ratio L_m(p0) / L_max corresponds to
 *     the desired confidence level.</li>
 * </ol>
 *
 * L_m(p_0) is sometimes called the "profiled likelihood function" or the "reduced likelihood function",
 * as it only depends on one of the original n+1 parameters.
 */
class DeltaNLLIntervalProducer: public theta::Producer{
public:
    /** DeltaNLL-producer for interval of parameter \c pid with confidence levels
     *  \c clevels_, using the minimizer \c min.
     *
     *  Note that ownership of min will be taken, i.e., \c min.get()==0 holds after a call
     *  to this constructor.
     */
    DeltaNLLIntervalProducer(const theta::ParId & pid_, const std::vector<double> & clevels_, std::auto_ptr<theta::Minimizer> & min,
            const std::string & name_);

    /** \brief Run the statistical method with Data and model and write out the result
     *    to the database.
     *
     * As this producer can be configured to determine several intervals at once, it
     * usually makes more than one entry per pseudo experiment in the result table.
     */
    virtual void produce(theta::Run & run, const theta::Data & data, const theta::Model & model);
private:
    theta::ParId pid;
    std::vector<double> clevels;
    std::auto_ptr<theta::Minimizer> minimizer;
    //at construction, save here the deltanll values corresponding to
    //clevels:
    std::vector<double> deltanll_levels;
    DeltaNllIntervalTable table;
};

/** \brief factory class for the class DeltaNLLProducer.
 *
 * Configuration is done with a settings block like:
 * <pre>
 * {
 *  type = "delta-nll";
 *  parameter = "p0";
 *  clevels = [0.68, 0.95];
 *  minimizer = "myminuit";
 * }
 *
 * myminuit = {...} //see the minimizer documentation.
 * </pre>
 *
 * \c type has always to be "delta-nll" in order to use this producer
 *
 * \c parameter is the name of the parameter for which the interval shall be calculated.
 *
 * \c clevels is an array (or list) of doubles of confidence levels for which the intervals
 *   shall be calculated. Note that an interval for the "0" confidence level (i.e., the
 *   interval containing the minimum) is always determined.
 *
 * \c minimizer is the configuration path to a \link theta::Minimizer Minimizer\endlink definition.
 *
 */
class DeltaNLLProducerFactory: public theta::plugin::ProducerFactory{
public:
    /** \brief implementation of the purely virtual build method of the factory.
     */
    virtual std::auto_ptr<theta::Producer> build(theta::plugin::ConfigurationContext & ctx) const;

    /** \brief always returns "delta-nll"
     */
    virtual std::string getTypeName() const{
        return "deltanll-interval";
    }
};

#endif

