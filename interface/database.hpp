#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include <sstream>
#include <iostream>
#include <stdlib.h>

#include <sqlite3.h>
#include <boost/utility.hpp>

#include "interface/variables.hpp"
#include "interface/decls.hpp"
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
 */
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

/** \brief Severity levels in for LogTable.
 * 
 * <ol>
 * <li>\c error should be used if a serious condition is reported which will likely
 *     lead to abnormal program termination or invalidation of the result.</li>
 * <li>\c warning should be used if a problem is not as serious as \c error, but might affect
 *     the result in an undesired way. </li>
 * <li>\c info is used purely informational, without the implicit action request of
 *    \c error and \c warning. It covers such entries like the start and end of a pseudo experiment. </li>
 * <li>\c debug is used like \c info, but for very detailed reporting which is usually not required
 *   and can be turned off for a whole run (whereas logging on other levels is always on).</li>
 * </ol>
 **/
enum e_severity {
    error = 0, warning, info, debug
};
}

/** \brief Central table to store any logging information.
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
     * \param db The Database the table should be created in.
     * \param name The name of the table. Has to pass Table::checkName()
     */
    LogTable(const std::string & name): Table (name){}
    
    /** \brief Append message to log table.
     * 
     * \param runid The run id the log entry is associted with.
     * \param eventid The event id the log entry is associted with or 0 if no particular event is referred.
     * \param message The log message, in human-readable english.
     * \param force_write If \c true, a new transaction is started, forcing an immidiate flush-to-disk.
     *  This is useful if an abnormal program termination is likely and the log entry should be written before that.
     */
    void append(const theta::Run & run, severity::e_severity s, const std::string & message, bool force_write = false);

private:
    /** \brief Implementation of Table::create_table to do the actual table creation.
     */
    virtual void create_table();
};


/**\brief Table to store per-run information about all active producers.
 *
 * This table object is used by an instance of \link theta::Run Run \endlink.
 * 
 * The corresponding SQL table has following fields:
 * <ol>
 * <li>\c runid \c INTEGER(4): the run id the entry refers to.</li>
 * <li>\c ind \c INTEGER(4): the index (starting from 0) for this producer in the current run configuration,
 *   i.e. the index it appeared in the producers = ("...") list in the configuration. </li>
 * <li>\c name \c TEXT: the name of the producer, as defined in the setting (via the setting name).</li>
 * <li>\c setting \c TEXT: configuration settings block of this producer.</li>
 * </ol>
 */
class ProducerInfoTable: public Table {
public:
    /** \brief Construct a new producer table in Database \c db with name \c name.
     *
     * \param db The Database the table should be created in.
     * \param name The name of the table. Has to pass Table::checkName()
     */
    ProducerInfoTable(const std::string & name): Table(name){}
    
    /** \brief Append an entry to the ProducerInfoTable.
     * 
     * \param runid The run id.
     * \param index The index for this producer in the current run configuration.
     * \param p_name The name of the producer.
     * \param setting The configuration file settings block for this producer
     */
    void append(const theta::Run & run, int index, const std::string & p_name, const std::string & setting);

private:
    /** \brief Implementation of Table::create_table to do the actual table creation.
     */
    virtual void create_table();
};

/**\brief Table to store per-run information about the random number seed.
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

class MCMCQuantileTable: public Table {
public:
    MCMCQuantileTable(const std::string & name_) : Table(name_){}
    void append(const theta::Run & run, double quantile, double parvalue);
private:
    virtual void create_table();
};

class ParamTable: public Table {
public:

  void append(const theta::Run & run, const theta::ParValues & values, const std::map<theta::ObsId, double> n_data);

  /** Create a new table with name \c tablename in the given database.
   * The columns of the table are
   * - INT runid
   * - INT eventid
   * - DOUBLE param for each parameter in \c ids. These columns have the name of the parameter as defined in \c vm.
   * - DOUBLE n_data_&lt;obs&gt; for each observable. This column holds the integral of the pseudo events produced for this observable.
   */
  ParamTable(const std::string & name, const theta::VarIdManager & vm, const theta::ParIds & ids, const theta::ObsIds & obs_ids);
private:
    virtual void create_table();

    theta::ParIds par_ids;
    theta::ObsIds obs_ids;
    std::vector<std::string> pid_names;
    std::vector<std::string> oid_names;
};

}//namespace database

#endif /* DATABASE_HPP */
