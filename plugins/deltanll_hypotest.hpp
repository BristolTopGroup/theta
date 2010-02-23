#ifndef PLUGIN_DELTANLL_HYPOTEST_HPP
#define PLUGIN_DELTANLL_HYPOTEST_HPP

#include "interface/plugin_so_interface.hpp"

#include "interface/variables.hpp"
#include "interface/database.hpp"

#include <string>


/** \brief Result table for the DeltaNLLHypotestProducer.
 *
 * The table contains the following columns:
 * <ol>
 * <li> runid INT(4)</li>
 * <li> eventid INT(4)</li>
 * <li> lnq DOUBLE</li>
 * </ol>
 *
 * For every event, one entry is made, containing the result of the DeltaNLLHypotestProducer. For more information
 * about the meaning, see documentation of DeltaNLLHypotestProducer.
 */
class DeltaNLLHypotestTable: public database::Table {
public:
  DeltaNLLHypotestTable(const std::string & name_): database::Table(name_){}
  void append(const theta::Run & run, double nll_sb, double nll_b);
private:
    virtual void create_table();
};


/** \brief A producer to create test statistics based on likelihood ratio.
 *
 * Given data and a model, this producer will find the maximum likelihood for a "signal-plus-background"
 * contraint and a "background-only" constraint and save the distance of the negative log-likelihood as 'lnq'
 * in the result table, DeltaNLLHypotestTable.
 */
class DeltaNLLHypotestProducer: public theta::Producer{
public:
    DeltaNLLHypotestProducer(const theta::ParValues & s_plus_b_, const theta::ParValues & b_only_, std::auto_ptr<theta::Minimizer> & min, const std::string & name_);
    virtual void produce(theta::Run & run, const theta::Data & data, const theta::Model & model);
private:
    theta::ParValues s_plus_b;
    theta::ParValues b_only;
    std::auto_ptr<theta::Minimizer> minimizer;
    theta::ParIds par_ids_constraints;
    DeltaNLLHypotestTable table;
};


class DeltaNLLHypotestProducerFactory: public theta::plugin::ProducerFactory{
public:
    virtual std::auto_ptr<theta::Producer> build(theta::plugin::ConfigurationContext & ctx) const;
    virtual std::string getTypeName() const{
        return "deltanll-hypotest";
    }
};

#endif
