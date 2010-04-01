#ifndef PLUGIN_PSEUDODATA_WRITER_HPP
#define PLUGIN_PSEUDODATA_WRITER_HPP

#include "interface/decls.hpp"
#include "interface/database.hpp"
#include "interface/producer.hpp"
#include "interface/variables.hpp"

#include <vector>
#include <string>

/** \brief Producer to save the pseudo data used in the pseudo experiments
 *
 * This is meant mainly for cross checks, visualization and debugging. The producer should
 * not be used in large-scale production as it will save a lot of data.
 *
 * Configuration is done with a settings block like:
 * <pre>
 * {
 *  type = "pseudodata_writer";
 *  observables = ("o0", "o1");
 *  write-data = true;
 * }
 * </pre>
 *
 * \c type has always to be "pseudodata_writer" in order to use this producer
 *
 * \c observables is a list of observable names for which to write out the pseudodata information
 *
 * \c write-data is a boolean which specifies whether or not to actually write the data: if \c true, the
 *    data will be saved as a SQL-blob for each observable (see PseudodataTable). If set to \c false, only
 *    the number of events will be saved for each event.
 *
 * For each observable, the result table will contain a column of the name "n_events_&lt;observable name&gt;" and -- if \c write-data
 * is true -- a column "data_&lt;observable name&gt;" with the histogram data as BLOB. This blob contains the raw double
 * array of the histogram, including underflow and overflow.
 */
class pseudodata_writer: public theta::Producer {
public:

    /// \brief Constructor used by the plugin system to build an instance from settings in a configuration file
    pseudodata_writer(const theta::plugin::Configuration & cfg);

    /** \brief Run the writer and write out the pseudo data \c Data to the database.
     */
    virtual void produce(theta::Run & run, const theta::Data & data, const theta::Model & model);
    
    /// Define the columns as described in the class documentation
    virtual void define_table();
private:
    boost::shared_ptr<theta::VarIdManager> vm;
    std::vector<theta::ObsId> observables;
    std::vector<theta::EventTable::column> n_events_columns;
    std::vector<theta::EventTable::column> data_columns;
    bool write_data;
};

#endif
