#include <sqlite3.h>

#include <string>
#include <sstream>
#include <iostream>
#include <stdlib.h>

#include <boost/filesystem.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include "interface/variables.hpp"
#include "interface/database.hpp"
#include "interface/run.hpp"
#include "interface/histogram.hpp"


using namespace std;
using namespace theta;

namespace fs = boost::filesystem;

Database::Database(const string & filename, bool autostart_transaction) :
    db(0), transaction_active(false) {
    if (fs::exists(filename)) {
        fs::remove(filename);
    }
    int res = sqlite3_open(filename.c_str(), &db);
    if (res != SQLITE_OK) {
        stringstream ss;
        ss << "Database::Database(filename=\"" << filename << "\")";
        error(ss.str());//throws.
    }
    if(autostart_transaction){
        beginTransaction();
    }
}

void Database::close() {
    if (!db)
        return;
    //end any active transaction:
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

Database::~Database() {
    //Close, but do not throw on failure, just print it:
    try {
        close();
    } catch (Exception & e) {
        cerr << "Exception while closing database in destructor: " << e.message << endl << "Ingoring." << endl;
    }
}

void Database::beginTransaction() {
    if(!transaction_active){
        exec("BEGIN;");
        transaction_active = true;
    }
}

void Database::endTransaction() {
    if (transaction_active)
        exec("END;");
    transaction_active = false;
}

void Database::exec(const string & query) {
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

sqlite3_stmt* Database::prepare(const string & sql) {
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

void Database::error(const string & functionName) {
    stringstream ss;
    ss << "Error in function " << functionName << ": Database said '" << sqlite3_errmsg(db) << "'";
    throw DatabaseException(ss.str());
}

Table::Table(const string & name_, const boost::shared_ptr<Database> & db_) :
    name(name_), table_created(false), insert_statement(0), next_column(1), db(db_) {
    checkName(name);
}

void Table::checkName(const string & name) {
    if (name.size() == 0)
        throw DatabaseException("Table::checkName: name was empty.");
    if (not ((name[0] >= 'A' && name[0] <= 'Z') or (name[0] >= 'a' && name[0] <= 'z'))) {
        stringstream ss;
        ss << "Table::checkName: name '" << name << "' does not start with a letter as is required.";
        throw DatabaseException(ss.str());
    }
    for (size_t i = 1; i < name.size(); i++) {
        if (name[i] >= 'A' && name[i] <= 'Z')
            continue;
        if (name[i] >= 'a' && name[i] <= 'z')
            continue;
        if (name[i] >= '0' && name[i] <= '9')
            continue;
        if (name[i] == '-' || name[i] == '_')
            continue;
        stringstream ss;
        ss << "Table::validateName: name '" << name << "' invalid at letter " << i;
        throw DatabaseException(ss.str());
    }
}

Table::column Table::add_column(const std::string & name, const data_type & type){
    if(table_created) throw IllegalStateException("Table::add_column called after table already created (via call to set_column / add_row).");
    checkName(name);
    column_definitions << ", '" << name << "' ";
    switch(type){
        case typeDouble: column_definitions << "DOUBLE"; break;
        case typeInt: column_definitions << "INTEGER(4)"; break;
        case typeString: column_definitions << "TEXT"; break;
        case typeBlob: column_definitions << "BLOB"; break;
        default:
            throw InvalidArgumentException("Table::add_column: invalid type parameter given.");
    };
    return next_column++;
}

void Table::create_table(){
    stringstream ss;
    string col_def = column_definitions.str();
    col_def.erase(0, 1);//delete first ','
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
Table::~Table(){
    if(not table_created) create_table();
}


void Table::set_column(const Table::column & c, double d){
    if(not table_created) create_table();
    sqlite3_bind_double(insert_statement, c, d);
}

void Table::set_column(const Table::column & c, int i){
    if(not table_created) create_table();
    sqlite3_bind_int(insert_statement, c, i);
}

void Table::set_column(const Table::column & c, const std::string & s){
    if(not table_created) create_table();
    sqlite3_bind_text(insert_statement, c, s.c_str(), s.size(), SQLITE_TRANSIENT);
}

void Table::set_column(const Table::column & c, const void * data, size_t nbytes){
    if(not table_created) create_table();
    sqlite3_bind_blob(insert_statement, c, data, nbytes, SQLITE_TRANSIENT);
}

void Table::add_row(){
    int res = sqlite3_step(insert_statement);
    sqlite3_reset( insert_statement);
    //reset all to NULL
    sqlite3_clear_bindings(insert_statement);
    if (res != 101) {
        throw DatabaseException("Table::add_row: sql returned error"); //TODO: more error info
    }
}

/* ProducerTable */
EventTable::EventTable(const std::string & name, const boost::shared_ptr<Database> & db): Table(name, db){
    c_runid = Table::add_column("runid", typeInt);
    c_eventid = Table::add_column("eventid", typeInt);
}

EventTable::column EventTable::add_column(const plugin::PluginType & p, const std::string & name, const data_type & type){
    std::stringstream new_name;
    new_name << p.get_name() << "__" << name;
    return Table::add_column(new_name.str(), type);
}

void EventTable::add_row(const Run & run){
    set_column(c_runid, run.get_runid());
    set_column(c_eventid, run.get_eventid());
    Table::add_row();
}

/* LogTable */
LogTable::LogTable(const std::string & name, const boost::shared_ptr<Database> & db): Table (name, db), level(info){
   for(int i=0; i<4; ++i){
       n_messages[i]=0;
   }
   c_runid = add_column("runid", typeInt);
   c_eventid = add_column("eventid", typeInt);
   c_severity = add_column("severity", typeInt);
   c_message = add_column("message", typeString);
   c_time = add_column("time", typeDouble);
}

const int* LogTable::get_n_messages() const{
    return n_messages;
}
    

void LogTable::set_loglevel(e_severity s){
    level = s;
}

LogTable::e_severity LogTable::get_loglevel() const{
    return level;
}

void LogTable::really_append(const Run & run, e_severity s, const string & message) {
    n_messages[s]++;
    set_column(c_runid, run.get_runid());
    set_column(c_eventid, run.get_eventid());
    set_column(c_severity, s);
    set_column(c_message, message);
    using namespace boost::posix_time;
    using namespace boost::gregorian;
    ptime t(microsec_clock::universal_time());
    time_duration td = t - ptime(date(1970, 1, 1));
    double time = td.total_microseconds() / 1000000.0;
    set_column(c_time, time);
    add_row();
}

// ProducerInfoTable
ProducerInfoTable::ProducerInfoTable(const std::string & name, const boost::shared_ptr<Database> & db): Table(name, db){
    c_ind = add_column("ind", typeInt);
    c_name = add_column("name", typeString);
    c_type = add_column("type", typeString);
    c_info = add_column("info", typeString);
}
    
void ProducerInfoTable::append(int index, const std::string & p_name, const std::string & p_type, const std::string & info){
    set_column(c_ind, index);
    set_column(c_name, p_name);
    set_column(c_type, p_type);
    set_column(c_info, info);
    add_row();
}

//RndInfoTable
RndInfoTable::RndInfoTable(const std::string & name_, const boost::shared_ptr<Database> & db): Table(name_, db){
    c_runid = add_column("runid", typeInt);
    c_seed = add_column("seed", typeInt);
}

void RndInfoTable::append(const Run & run, int seed){
    set_column(c_runid, run.get_runid());
    set_column(c_seed, seed);
    add_row();
}

//ParamTable
ParamTable::ParamTable(const std::string & name_, const boost::shared_ptr<Database> & db, const theta::VarIdManager & vm, const theta::ParIds & ids):
        Table(name_, db), par_ids(ids){
    c_runid = add_column("runid", typeInt);
    c_eventid = add_column("eventid", typeInt);
    columns.reserve(par_ids.size());
    for(ParIds::const_iterator it=ids.begin(); it!=ids.end(); ++it){
        columns.push_back(add_column(vm.getName(*it), typeDouble));
    }
}

void ParamTable::append(const Run & run, const ParValues & values) {
    set_column(c_runid, run.get_runid());
    set_column(c_eventid, run.get_eventid());
    int icol = 0;
    for (ParIds::const_iterator it = par_ids.begin(); it != par_ids.end(); ++it, ++icol) {
        set_column(columns[icol], values.get(*it));
    }
    add_row();
}

