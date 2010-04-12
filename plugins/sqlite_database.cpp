#include "plugins/sqlite_database.hpp"
#include "interface/plugin.hpp"

#include <sstream>

#include <boost/filesystem.hpp>

using namespace std;
using namespace theta;

sqlite_database::sqlite_database(const plugin::Configuration & cfg) :
    db(0), transaction_active(false){
    std::string filename = cfg.setting["filename"];
    if (boost::filesystem::exists(filename)) {
        boost::filesystem::remove(filename);
    }
    int res = sqlite3_open(filename.c_str(), &db);
    if (res != SQLITE_OK) {
        stringstream ss;
        ss << "sqlite_database constructor failed (filename=\"" << filename << "\") with SQLITE code " << res;
        error(ss.str());//throws.
    }
    beginTransaction();
}

void sqlite_database::close() {
    if (!db)
        return;
    endTransaction();
    //finalize all statements associated with the connection:
    sqlite3_stmt *pStmt;
    while ((pStmt = sqlite3_next_stmt(db, 0)) != 0) {
        sqlite3_finalize(pStmt);
    }
    int res = sqlite3_close(db);
    //set db to zero even in case of failure as it is unlikely
    // that the error is recoverable ...
    db = 0;
    if (res != 0) {
        stringstream ss;
        ss << "Database::close(): sqlite3_close() returned " << res;
        throw DatabaseException(ss.str());
    }
}

sqlite_database::~sqlite_database() {
    //Close, but do not throw on failure, just print it:
    try {
        close();
    } catch (Exception & e) {
        cerr << "Exception while closing database in destructor: " << e.message << endl << "Ingoring." << endl;
    }
}

void sqlite_database::beginTransaction() {
    if(!transaction_active){
        exec("BEGIN;");
        transaction_active = true;
    }
}

void sqlite_database::endTransaction() {
    if (transaction_active)
        exec("END;");
    transaction_active = false;
}

void sqlite_database::exec(const string & query) {
    char * err = 0;
    sqlite3_exec(db, query.c_str(), 0, 0, &err);
    if (err != 0) {
        stringstream ss;
        ss << "Database.exec(\"" << query << "\") returned error: " << err;
        sqlite3_free(err);
        //database errors should not happen at all. If they do, we cannot use the log table, so write the error
        // to stderr, so the user knows what has happened:
        cerr << "SQL error: " << ss.str() << endl;
        throw DatabaseException(ss.str());
    }
}

sqlite3_stmt* sqlite_database::prepare(const string & sql) {
    sqlite3_stmt * statement = 0;
    int ret = sqlite3_prepare_v2(db, sql.c_str(), sql.size() + 1, &statement, 0);
    if (ret != SQLITE_OK) {
        if (statement != 0) {
            sqlite3_finalize(statement);
        }
        try {
            error( __FUNCTION__);
        } catch (DatabaseException & ex) {
            stringstream ss;
            ss << ex.message << " (return code " << ret << "); SQL was " << sql;
            throw DatabaseException(ss.str());
        }
    }
    return statement;
}

void sqlite_database::error(const string & functionName) {
    stringstream ss;
    ss << "Error in function " << functionName << ": Database said '" << sqlite3_errmsg(db) << "'";
    throw DatabaseException(ss.str());
}

std::auto_ptr<Table> sqlite_database::create_table(const string & table_name){
    check_name(table_name);
    return std::auto_ptr<Table>(new sqlite_table(table_name, this));
}


sqlite_database::sqlite_table::sqlite_table(const string & name_, sqlite_database * db_) :
    name(name_), table_created(false), insert_statement(0), next_column(1), db(db_) {
}

std::auto_ptr<Column> sqlite_database::sqlite_table::add_column(const std::string & name, const data_type & type){
    if(table_created) throw IllegalStateException("Table::add_column called after table already created (via call to set_column / add_row).");
    column_definitions << (next_column>1?", '":"'") << name << "' ";
    switch(type){
        case typeDouble: column_definitions << "DOUBLE"; break;
        case typeInt: column_definitions << "INTEGER(4)"; break;
        case typeString: column_definitions << "TEXT"; break;
        case typeBlob: column_definitions << "BLOB"; break;
        default:
            throw InvalidArgumentException("Table::add_column: invalid type parameter given.");
    };
    std::auto_ptr<Column> result(new sqlite_column(next_column));
    next_column++;
    return result;
}

void sqlite_database::sqlite_table::create_table(){
    stringstream ss;
    string col_def = column_definitions.str();
    ss << "CREATE TABLE '" << name << "' (" << col_def << ");";
    db->exec(ss.str());
    ss.str("");
    ss << "INSERT INTO '" << name << "' VALUES(";
    for(int i=1; i<next_column; ++i){
        if(i > 1) ss << ", ?";
        else ss << "?";
    }
    ss << ");";
    insert_statement = db->prepare(ss.str());
    table_created = true;
}

//create the table if it is empty to ensure that all tables have been created
// even if there are no entries
sqlite_database::sqlite_table::~sqlite_table(){
    if(not table_created) create_table();
}

void sqlite_database::sqlite_table::set_column(const Column & c, double d){
    if(not table_created) create_table();
    sqlite3_bind_double(insert_statement, static_cast<const sqlite_column&>(c).sqlite_column_index, d);
}

void sqlite_database::sqlite_table::set_column(const Column & c, int i){
    if(not table_created) create_table();
    sqlite3_bind_int(insert_statement, static_cast<const sqlite_column&>(c).sqlite_column_index, i);
}

void sqlite_database::sqlite_table::set_column(const Column & c, const std::string & s){
    if(not table_created) create_table();
    sqlite3_bind_text(insert_statement, static_cast<const sqlite_column&>(c).sqlite_column_index, s.c_str(), s.size(), SQLITE_TRANSIENT);
}

void sqlite_database::sqlite_table::set_column(const Column & c, const void * data, size_t nbytes){
    if(not table_created) create_table();
    sqlite3_bind_blob(insert_statement, static_cast<const sqlite_column&>(c).sqlite_column_index, data, nbytes, SQLITE_TRANSIENT);
}

void sqlite_database::sqlite_table::add_row(){
    if(not table_created) create_table();
    int res = sqlite3_step(insert_statement);
    sqlite3_reset( insert_statement);
    //reset all to NULL
    sqlite3_clear_bindings(insert_statement);
    if (res != 101) {
        throw DatabaseException("Table::add_row: sql returned error"); //TODO: more error info
    }
}

REGISTER_PLUGIN(sqlite_database)
