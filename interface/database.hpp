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

/** \brief Namespace for the Database and Table objects to save any data created during a run.
 */
namespace database {

/** \brief Used as abstraction to a %database file, where one or more tables can reside.
 * 
 * There is a one-to-one correspondence between a file on the file system and a "Database" object.
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
     * */
    void exec(const std::string & query);
    
    /** Prepare the sql statement \c query.
     * */
    sqlite3_stmt* prepare(const std::string & query);
    
    /** Start a Transaction. In case of an error, a DatabaseException is thrown.
     * */
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
 * Classes derived from this abstract class usually implement -- apart from a
 * constructor -- only two methods:
 * <ol>
 * <li>the virtual method create_table() which created the actual table in the databse instance (note that
 *   at construction time, the Database instance might not yet be available. Therefore, the actual
 *   table creation is not done in the constructor)</li>
 * <li>an \c append() method which inserts a new row at the end of the table.
 *     The signature of this method depends on the fields for the particular table.</li>
 * </ol>
 *
 * Any producer table MUST have, as first two fields:
 * <ol>
 *   <li>runid INT(4)</li>
 *   <li>eventid INT(4)</li>
 * </ol>
 * and write the current runid and eventid of the event for which the result was produced to the table.
 */
//TODO: This runid, eventod column requirement should be enforced by design. For example,
// Table could provide protected methods
//   column addColumn(string, data_type::type) (called at the beginning)
//then, repeatedly:
//    void setColumn(column, value) for each column and
//    void addRow() for each row to add, after setting all columns
//which provide an abstraction to SQL.
// namespace data_type { enum type { double_, int_, string_, blob_ }; }
class Table: private boost::noncopyable {
    /** Validate table (or column) names. Throws a theta::DatabaseException if the name
     * is not considered a valid name for a table or column.
     * valid names start with a letter and are only composed of
     * A-Za-z0-9_-
     * This is (almost) the same as the name of Settings accepted by libconfig.
     */
    static void checkName(const std::string & name);
    
public:

    /** \brief Returns whether the setup was run correctly.
     *
     * After a table instance has been created, it is <b>not</b> automatically
     * associated with a particular Databse instance. Rather, connect() has to be
     * called before writing data to the table. To check whether this setup has already run,
     * use this function.
     */
    operator bool(){
        return db.get()!=0;
    }

    /** \brief setup a connection to a Database instance.
     *
     * It is not valid to call this method twice on the same table instance
     * (be it the same or a different Database). In such a case, an
     * IllegalStateException will be thrown.
     */
    void connect(const boost::shared_ptr<Database> & db_){
        //if already connected: throw
        if(*this){
            throw theta::IllegalStateException("Table::connect: table was already connected!");
        }
        //well, maybe the word "connection" is an exaggeration ...
        db = db_;
        create_table();
    }
    
protected:
    /** \brief create the tables.
     *
     * This function has to be overridden by derived classes which will create
     * their tables here.
     */
    virtual void create_table() = 0;

    /// constructor setting the table name.
    Table(const std::string & name_);

    /// default destructor declared virtual, as polymorphic access to derived classes might occur
    virtual ~Table() {}
    
    /// Proxy to Database::exec for derived classes.
    void exec(const std::string & query){
        db->exec(query);
    }
    /// Proxy to Database::beginTransaction
    void beginTransaction(){
        db->beginTransaction();
    }
    
    ///Proxy to Database::endTransaction
    void endTransaction(){
        db->endTransaction();
    }    
    
    /// Proxy to Database::prepare for derived classes.
    sqlite3_stmt* prepare(const std::string & query){
        return db->prepare(query);
    }

    /// Proxy to Database::error for derived classes.
    void error(const std::string & functionName){
        db->error(functionName);
    }

    /// the name of the table in the database
    std::string name;
    /// a pointer to the database object. Is zero
    boost::shared_ptr<Database> db;
    /// the insert_statement for the table. Should be created by create_table()
    sqlite3_stmt * insert_statement; //associated memory is managed by Database object, which calls finalize on any
    //associated sqlite3_stmt.
};


namespace severity {

/** \brief Severity levels in for LogTable
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
}

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
    /** \brief Construct a new table in Database \c db with name \c name.
     *
     * The default loglevel is warning.
     *
     * \param name The name of the table. Has to pass Table::checkName()
     */
    LogTable(const std::string & name);
    
    /** \brief Set the current log level.
     *
     * Future calls to append() will only write the message to the table if the messages
     * has a severity which is euqal to or exceeds the level given here.
     *
     * Note that is it not possible to disable logging of error messages.
     */
    void set_loglevel(severity::e_severity s);
    
    /** \brief Get the currently active log level
     */
    severity::e_severity get_loglevel() const;
    
    /** \brief Append message to log table, if severity is larger than currently configured level
     * 
     * \param run The Run instance to ask for the current runid and eventid
     * \param s The severity level of the log message
     * \param message The log message, in human-readable english
     */
    void append(const theta::Run & run, severity::e_severity s, const std::string & message){
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
    /** \brief Implementation of Table::create_table to do the actual table creation.
     */
    virtual void create_table();
    
    /// really append the log message. Called from append() in case severity is large enough
    void really_append(const theta::Run & run, severity::e_severity s, const std::string & message);
    
    severity::e_severity level;
    int n_messages[4];
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
     * \param name The name of the table. Has to pass Table::checkName()
     */
    ProducerInfoTable(const std::string & name): Table(name){}
    
    /** \brief Append an entry to the ProducerInfoTable.
     * 
     * \param index The index for this producer in the current run configuration
     * \param p_name The name of the producer
     * \param p_type The right hand side of the type="..."; setting used to configure this producer
     * \param info The information of the producer (result of Producer::get_information())
     */
    void append(int index, const std::string & p_name, const std::string & p_type, const std::string & info);

private:
    /** \brief Implementation of Table::create_table to do the actual table creation.
     */
    virtual void create_table();
};

/** \brief Table to store per-run information about the random number seed.
 *
 * This table object is used by an instance of \link theta::Run Run \endlink.
 *
 * The corresponding SQL table has following fields:
 * <ol>
 * <li>\c runid \c INTEGER(4): the run id the entry refers to.</li>
 * <li>\c seed \c INTEGER(8): the seed of the random number generator used in the run.</li>
 * </ol>
 */
class RndInfoTable: public Table {
public:
    /** \brief Construct a new RndInfoTable with name \c name.
     */
    RndInfoTable(const std::string & name_): Table(name_){}

    /** \brief append an entry to the RndInfoTable
     *
     * \param run is the current Run. It is used to query the current runid
     * \param seed is the seed to save in the table
     */
    void append(const theta::Run & run, long seed);
private:
    /** \brief Implementation of Table::create_table to do the actual table creation.
     */
    virtual void create_table();
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
   * \param vm is used to query the parameter names, which are used as column names
   * \param ids the parameter ids which to write in the table (usually all from the pseudodata Model)
   */
  ParamTable(const std::string & name, const theta::VarIdManager & vm, const theta::ParIds & ids);
private:
    virtual void create_table();

    theta::ParIds par_ids;
    std::vector<std::string> pid_names;
};

}//namespace database

#endif /* DATABASE_HPP */
