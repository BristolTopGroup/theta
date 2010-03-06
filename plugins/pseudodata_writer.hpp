#ifndef PLUGIN_PSEUDODATA_WRITER_HPP
#define PLUGIN_PSEUDODATA_WRITER_HPP

#include "interface/decls.hpp"
#include "interface/database.hpp"
#include "interface/producer.hpp"

//#include "interface/plugin_so_interface.hpp"


#include "interface/variables.hpp"

#include <vector>
#include <string>


/** \brief Table to store results of a \link pseudodata_writer pseudodata_writer \endlink
 *
 * The corresponding SQL table has following fields:
 * <ol>
 *  <li>\c runid \c INTEGER(4): the run id the entry refers to</li>
 *  <li>\c eventid \c INTEGER(4): the event id the entry refers to</li>
 *  <li>For each observable: \c n_events_&lt;observable&gt; \c DOUBLE: the total number of events and \c data_&lt;observable&gt; \c BLOB : the data of the histogram</li>
 * </ol>
 * 
 * Note that the data_* columns might be missing depending on the configuration of \link pseudodata_writer pseudodata_writer \endlink
 *
 * n_events is a double rather than int as special runs might produce asimov data (i.e., not throw possion data around
 * some mean, but pass the mean itself as data).
 *
 * The \c data_* columns save a raw double array. It saves underflow or overflow bin, so for an
 * observable with 3 bins, 5 doubles will be saved.
 */
class PseudodataTable: public database::Table {
friend class pseudodata_writer;
public:
    
    /** \brief Construct a new PseudodataTable of the given name.
     */
    PseudodataTable(const std::string & name_): database::Table(name_){}

    /** \brief append an entry to the table
     *
     * \param run the theta::Run instance used to extract the current runid and eventid
     * \param data the pseudodata to write to the table
     */
    void append(const theta::Run & run, const theta::Data & data);
private:
    /** \brief Implementation of Table::create_table to do the actual table creation.
     */
    virtual void create_table();
    
    //write actually data (or only n_events)?
    bool write_data;
    //observables for which to write out data:
    theta::ObsIds observables;
    std::vector<std::string> observable_names;
};

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
 */
class pseudodata_writer: public theta::Producer {
public:

    /// \brief Constructor used by the plugin system to build an instance from settings in a configuration file
    pseudodata_writer(const theta::plugin::Configuration & cfg);

    /** \brief Run the writer and write out the pseudo data \c Data to the database.
     */
    virtual void produce(theta::Run & run, const theta::Data & data, const theta::Model & model);
private:
    PseudodataTable table;
};

#endif
