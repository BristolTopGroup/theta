#ifndef PLUGIN_SQLITE_DATABASE_HPP
#define PLUGIN_SQLITE_DATABASE_HPP

#include "interface/decls.hpp"
#include "interface/database.hpp"

#include <sqlite3.h>
#include <memory>
#include <string>


/** \brief Database which stores all information in a single sqlite3 database file
 *
 * Configured via a setting group like
 * \code
 * {
 *   type = "sqlite_database";
 *   filename = "abc.db";
 * }
 * \endcode
 *
 * \c type must always be "sqlite_database" in order to select this plugin
 *
 * \c filename is the filename of the sqlite3 output file. It is a path relative to the path where theta is invoked
 *
 * If the file already exists, it is overwritten silently.
 *
 * The types Table::typeDouble, Table::typeInt and Table::typeString are translated directly
 * to their SQL counterparts \c DOUBLE, \c INT(4) and \c TEXT, respectively. For Table::typeHisto,
 * an SQL BLOB is saved which contains the lower and upper border of the histogram and the raw histogram data,
 * including underflow and overflow bin (see theta::Histogram::getData).
 */
class sqlite_database: public theta::Database{
public:
    
    /** \brief Constructor for the plugin system
     *
     * See class documentation for a description of the parsed COnfiguration settings.
     */
    sqlite_database(const theta::plugin::Configuration & cfg);
    
    
    virtual ~sqlite_database();
    
    /** \brief See documentation of Database::create_table
     */
    virtual std::auto_ptr<theta::Table> create_table(const std::string & table_name);
        
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
    
    void close();
    
    sqlite3* db;
    bool transaction_active;
    
    
    //declare privately(!) the sqlite_table class:
    class sqlite_table: public theta::Table {
    friend class sqlite_database;

        // destructor; creates the table if empty
        virtual ~sqlite_table();
        
        virtual std::auto_ptr<theta::Column> add_column(const std::string & name, const data_type & type);
        virtual void set_autoinc_column(const std::string & name);
        
        virtual void set_column(const theta::Column & c, double d);
        virtual void set_column(const theta::Column & c, int i);
        virtual void set_column(const theta::Column & c, const std::string & s);
        virtual void set_column(const theta::Column & c, const theta::Histogram & h);
        virtual int add_row();

    private:
        
        sqlite_table(const std::string & name_, sqlite_database * db_);
        
        std::string name;
        bool have_autoinc;
        std::stringstream column_definitions; // use by the add_column method
        std::stringstream ss_insert_statement;
        bool table_created;
        
        int next_insert_index; // next free insert_index to use by add_column to construct an sqlite_column, starting at 1.
        
        sqlite3_stmt * insert_statement; // this ressource is owned by sqlite_database.
        sqlite_database * db;
        
        void create_table();
        
        class sqlite_column: public theta::Column{
        public:
            int insert_index;
            sqlite_column(int i):insert_index(i){}
            virtual ~sqlite_column(){}
        };
        
        class sqlite_autoinc_column: public theta::Column{
            virtual ~sqlite_autoinc_column(){}
        };
    };
};

#endif
