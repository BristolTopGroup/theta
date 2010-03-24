#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include <sstream>
#include <iostream>
#include <stdlib.h>

#include <sqlite3.h>
#include <boost/utility.hpp>

#include "interface/decls.hpp"
#include "interface/variables.hpp"
#include "interface/exception.hpp"

namespace theta {

/** \brief Used as abstraction to a database file, where one or more tables can reside
 * 
 * There is a one-to-one correspondence between a file on the file system and a Database instance.
 */
class Database: private boost::noncopyable {
    /** Table objects make use of the private methods \c exec, \c error, and \c prepare.
     * They also act as proxy class for derived table objects.
     */
    friend class Table;
public:
    
    /** Creates a database in a file specified by \c filename.
     *  If the file alread exists, it is deleted and newly created.
     *  
     *  In case of error, a \c theta::DatabaseException is thrown.
     *  
     *  \param filename The associated file name on the file system.
     *  \param autostart_transaction If set to \c true, start a SQL Transaction immidiately.
     */
    explicit Database(const std::string & filename, bool autostart_transaction=true);
    
    /** Closes the underlying database file just as \c close() would. However,
     * does not throw any exception. Rather, it prints an error message to \c cerr
     * in case of an error.
     * */
    ~Database();
    
    /** Commit any active transaction and close the attached database file.
     * 
     * Throws a theta::DatabaseException in case of an error. 
     * */
    void close();
        
private:
    /** Execute the sql string \c query.
     */
    void exec(const std::string & query);
    
    /** Prepare the sql statement \c query.
     */
    sqlite3_stmt* prepare(const std::string & query);
    
    /** Start a Transaction. In case of an error, a DatabaseException is thrown.
     */
    void beginTransaction();
    
    /** End a transaction previously started with \c beginTransaction. Calling
     *  endTransaction without a call to beginTransaction is valid (it is a no-op).
     * */
    void endTransaction();    

    /** Throw a DatabaseException using the last database error message.
     * 
     * This method is intended for Table objects which want to propagate
     * an error via Exceptions in a consistent manner.
     * 
     * \param functionName: name of the function (in a \c Table object) in which the database error ocurred.
     */
    void error(const std::string & functionName);
    sqlite3* db;
    bool transaction_active;
};


/** \brief Abstract base class for all tables stored in a Database.
 *
 * Writing information to the output database is done via subclasses of Table.
 * Producers always write their result in an instance of ProducerTable.
 *
 * A derived class uses this class by:
 * <ol>
 *   <li>calling add_column one or more times at initialization and saving the result
 *       in some data member of the derived class</li>
 *   <li>To write out a row, call the set_column methods on all defined columns,
 *       followed by a call to add_row</li>
 * </ol>
 */
class Table: private boost::noncopyable {
    /** \brief Validate table (or column) names.
     *
     * Throws a theta::DatabaseException if the name
     * is not considered a valid name for a table or column.
     *
     * Valid names start with a letter and are only composed of
     * <pre>
     * A-Za-z0-9_-
     * </pre>
     * only. This is (almost) the same as the name of Settings accepted by libconfig.
     */
    static void checkName(const std::string & name);

protected:

    /** \brief Data types of columns
     *
     * These data types are the supported types for columns in a table.
     */
    enum data_type { typeDouble, typeInt, typeString, typeBlob };

    /** \brief the columns type to use for set_column and add_column
     *
     * For now, we just use an integer, starting from 1. It will be passed to
     * the sqlite_bind-functions.
     */
    typedef int column;

    /// constructor setting the table name.
    Table(const std::string & name, const boost::shared_ptr<Database> & db);

    /// default destructor declared virtual, as polymorphic access to derived classes might occur
    virtual ~Table() {}
    
    /** \brief Add a column to the table
     *
     * The column will have name \c name and a data type scpecified by \c type.
     *
     * The result can be used as first argument to the set_column() methods. Other than saving it
     * and using it there, you should not do anything with it.
     *
     * This method should only be called at the beginning, after the construction of the object and
     * before using it to actually write any data. If this method is called after a call
     * to either set_column or add_row, an IllegalStateException will be thrown.
     */
    column add_column(const std::string & name, const data_type & type);
    
    //@{
    /** \brief Set column contents
     *
     * The first argument is a column as returned by add_column.
     *
     * After setting all columns, add_row() has to be called to actually write
     * a row to the result table.
     */
    void set_column(const column & c, double d);
    void set_column(const column & c, int i);
    void set_column(const column & c, const std::string & s);
    void set_column(const column & c, const void * data, size_t nbytes);
    //@}
    
    
    /** \brief Add a row to the table
     *
     * This uses the column values previously set with the set_column methods.
     * If set_column was not called for all columns in the table, the contents of these columns
     * will be NULL.
     */
    void add_row();

private:
    std::string name;
    std::stringstream column_definitions; // use by the add_column method
    bool table_created;
    sqlite3_stmt * insert_statement; // associated memory is managed by Database object, which calls finalize on it ...
    column next_column; // next free column to return by add_column
    boost::shared_ptr<Database> db;
    
    void create_table();
};

/** \brief A Table class to store per-event information
 *
 * Per \link run Run \endlink, there is exactly one EventTable. The main usages are
 * <ul>
 *   <li>by Producer to save the result</li>
 *   </li>by DataSource to save some per-event information about data production</li>
 * </ul>
 *
 * Clients define their columns by calling add_column repeatedly at initilization time.
 * The actual column name written to the SQL table is &lt;plugin name&gt;__&lt;column name&gt;.
 *
 * The actual write is done by the \link Run \endlink instance; the PluginType should
 * not call this method.
 */
class EventTable: private Table{
    public:
        //@{
        /** \brief Make some protected definitions from Table public
         */
        using Table::typeDouble;
        using Table::typeInt;
        using Table::typeString;
        using Table::typeBlob;
        using Table::column;
        using Table::set_column;
        //@}
        
        /** \brief Add a column to this table
         *
         * Similar to Table::add_column, but expects an instance of PluginType.
         * The column name will be
         *\code
         * p.get_name() + "__" + name
         *\endcode
         */
        column add_column(const theta::plugin::PluginType & p, const std::string & name, const data_type & type);
        
        /** \brief Construct a new producer table with a given name
         */
        EventTable(const std::string & name, const boost::shared_ptr<Database> & db);
        
        /** \brief Add a row to the table, given the current run
         *
         * This is called by theta::Run after a producer has executed.
         *
         * \c run will be used to fill the \c runid and \c eventid columns.
         */
        void add_row(const theta::Run & run);
        
    private:
        column c_runid, c_eventid;
};

/** \brief Table to store all logging information
 *
 * The corresponding SQL table has following fields:
 * <ol>
 * <li>runid INTEGER(4)</li>
 * <li>eventid INTEGER(4)</li>
 * <li>severity INTEGER(4)</li>
 * <li>message TEXT</li>
 * <li>time DOUBLE</li>
 * </ol>
 * 
 * \c runid and \c eventid are the run and event id, respectively, the log entry
 * is associated to. \c eventid is set to 0, if no particular event (but the run as a whole)
 * is referred to.
 * 
 * \c severity is one level from e_severity. See there for a description of the meaning
 * of these levels.
 * 
 * \c message is the human-readable log message.
 * 
 * \c time is the number of seconds since the unix epoch (1970-01-01 UTC), but
 *    with sub-second accuracy.
 **/
class LogTable: public Table {
public:
    /** \brief Severity levels for log messages
     * 
     * <ol>
     * <li>\c error should be used if a serious condition is reported which will likely
     *     affect the whole result.</li>
     * <li>\c warning should be used if a problem is not as serious as \c error, but might affect
     *     the result in an undesired way. </li>
     * <li>\c info is used purely informational, without the implicit action request of
     *    \c error and \c warning. It covers such events like the start and end of a pseudo experiment. </li>
     * <li>\c debug is used like \c info, but for very detailed reporting which is usually not required.</li>
     * </ol>
     **/
    enum e_severity {
        error = 0, warning, info, debug
    };
    
    /** \brief Construct a new table in Database \c db with name \c name.
     *
     * The default loglevel is warning.
     *
     * \param name The name of the table. Has to pass Table::checkName()
     * \param db The Database to create the table in
     */
    LogTable(const std::string & name, const boost::shared_ptr<Database> & db);
    
    /** \brief Set the current log level.
     *
     * Future calls to append() will only write the message to the table if the messages
     * has a severity which is euqal to or exceeds the level given here.
     *
     * Note that is it not possible to disable logging of error messages.
     */
    void set_loglevel(e_severity s);
    
    /** \brief Get the currently active log level
     */
    e_severity get_loglevel() const;
    
    /** \brief Append message to log table, if severity is larger than currently configured level
     * 
     * \param run The Run instance to ask for the current runid and eventid
     * \param s The severity level of the log message
     * \param message The log message, in human-readable english
     */
    void append(const theta::Run & run, e_severity s, const std::string & message){
        //define inline as hint to the compiler; keep this function as short as possible to
        // encourage the compiler actually inlining it and to have high performance benefits in
        // case the user disables logging.
        if(s <= level) really_append(run, s, message);
    }
    
    /** \brief Returns the number of messages
     *
     * Use severity::e_severity converted to int to access the number of messages written to the log.
     * Messages suppressed by the set_loglevel() method <em>not</em> counted.
     */
    const int* get_n_messages() const;

private:

    /// really append the log message. Called from append() in case severity is large enough
    void really_append(const theta::Run & run, e_severity s, const std::string & message);
    e_severity level;
    int n_messages[4];
    column c_runid, c_eventid, c_severity, c_message, c_time;
};


/** \brief Table to store information about all producers
 *
 * This table object is used by an instance of \link theta::Run Run \endlink.
 * 
 * The corresponding SQL table has following fields:
 * <ol>
 * <li>\c ind \c INTEGER(4): the index (starting from 0) for this producer in the current run configuration,
 *   i.e. the index it appeared in the producers = ("...") list in the configuration. </li>
 * <li>\c type \c TEXT: the type setting used to configure this producer, as given in the type="..."  setting for this producer</li> 
 * <li>\c name \c TEXT: the name of the producer, as defined in the setting (via the setting name).</li>
 * <li>\c info \c TEXT: a info field defined by the producer; see Producer::get_comment()</li> 
 * </ol>
 *
 * This table is the only table without a "runid" entry as its contents information is not run-dependent
 * and each database file only contains information with consistent producers.
 */
class ProducerInfoTable: public Table {
public:
    /** \brief Construct a new producer table in with name \c name.
     *
     * See Table::Table.
     */
    ProducerInfoTable(const std::string & name, const boost::shared_ptr<Database> & db);
    
    /** \brief Append an entry to the ProducerInfoTable.
     * 
     * \param index The index for this producer in the current run configuration
     * \param p_name The name of the producer
     * \param p_type The right hand side of the type="..."; setting used to configure this producer
     * \param info The information of the producer (result of Producer::get_information())
     */
    void append(int index, const std::string & p_name, const std::string & p_type, const std::string & info);

private:
    column c_ind, c_type, c_name, c_info;
};

/** \brief Table to store per-run information about the random number seed.
 *
 * This table object is used by an instance of \link theta::Run Run \endlink.
 *
 * The corresponding SQL table has following fields:
 * <ol>
 * <li>\c runid \c INTEGER(4): the run id the entry refers to.</li>
 * <li>\c seed \c INTEGER(4): the seed of the random number generator used in the run.</li>
 * </ol>
 */
class RndInfoTable: public Table {
public:
    /** \brief Construct a new RndInfoTable with name \c name.
     */
    RndInfoTable(const std::string & name_, const boost::shared_ptr<Database> & db);

    /** \brief append an entry to the RndInfoTable
     *
     * \param run is the current Run. It is used to query the current runid
     * \param seed is the seed to save in the table
     */
    void append(const theta::Run & run, int seed);
private:
    column c_runid, c_seed;
};

/** \brief Table to store per-event information about which parameter values have been used to create pseudo data
 *
 * This table object is used by an instance of \link theta::Run Run \endlink.
 *
 * The corresponding SQL table has following fields:
 * <ol>
 * <li>\c runid \c INTEGER(4): the run id the entry refers to.</li>
 * <li>\c eventid \c INTEGER(4): the event id the entry refers to.</li>
 * <li>for each parameter a column &lt;param_name&gt; DOUBLE with the parameter value used.</li>
 * </ol>
 */
class ParamTable: public Table {
public:
  /** \brief Append a new entry to the table
   *
   * \param run is the current Run. It is used to query the current runid and eventid
   * \param values contain the parameter values to write to the table.
   */
  void append(const theta::Run & run, const theta::ParValues & values);

  /** \brief Construct a ParamTable
   *
   * \param name is the table name in the database
   * \param db is the Database instance to create the table in
   * \param vm is used to query the parameter names, which are used as column names
   * \param ids the parameter ids which to write in the table (usually all from the pseudodata Model)
   */
  ParamTable(const std::string & name, const boost::shared_ptr<Database> & db, const theta::VarIdManager & vm, const theta::ParIds & ids);
private:
    theta::ParIds par_ids;
    column c_runid, c_eventid;
    std::vector<column> columns;
};

}

#endif
