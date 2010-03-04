#include <sqlite3.h>

#include <string>
#include <sstream>
#include <iostream>
#include <stdlib.h>

#include <boost/filesystem.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include "interface/variables.hpp"
#include "interface/database.hpp"
#include "interface/plugin_so_interface.hpp"
#include "interface/run.hpp"
#include "interface/histogram.hpp"


using namespace std;
using namespace theta;
using namespace database;

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

Table::Table(const string & name_) :
    name(name_), insert_statement(0) {
    Table::checkName(name);
}

void Table::checkName(const string & name) {
    if (name.size() == 0)
        throw DatabaseException("Table::validateName: name was empty.");
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

LogTable::LogTable(const std::string & name): Table (name), level(severity::info){
   for(int i=0; i<4; ++i){
       n_messages[i]=0;
   }
}

const int* LogTable::get_n_messages() const{
    return n_messages;
}
    

void LogTable::set_loglevel(severity::e_severity s){
    level = s;
}

severity::e_severity LogTable::get_loglevel() const{
    return level;
}

void LogTable::really_append(const Run & run, severity::e_severity s, const string & message) {
    n_messages[s]++;
    sqlite3_bind_int(insert_statement, 1, run.get_runid());
    sqlite3_bind_int(insert_statement, 2, run.get_eventid());
    sqlite3_bind_int(insert_statement, 3, s);
    sqlite3_bind_text(insert_statement, 4, message.c_str(), -1, SQLITE_TRANSIENT);
    using namespace boost::posix_time;
    using namespace boost::gregorian;
    ptime t(microsec_clock::universal_time());
    time_duration td = t - ptime(date(1970, 1, 1));
    double time = td.total_microseconds() / 1000000.0;
    sqlite3_bind_double(insert_statement, 5, time);
    int res = sqlite3_step(insert_statement);
    sqlite3_reset( insert_statement);
    if (res != 101) {
        error(__FUNCTION__);//throws exception
    }
}

void LogTable::create_table(){
    stringstream ss;
    ss << "CREATE TABLE '" << name << "' (runid INTEGER(4), eventid INTEGER(4), severity INTEGER(1), message TEXT, time DOUBLE);";
    exec(ss.str().c_str());
    ss.str("");
    ss << "INSERT INTO '" << name << "' VALUES(?,?,?,?,?);";
    insert_statement = prepare(ss.str());
}


void ProducerInfoTable::create_table() {
    stringstream ss;
    ss << "CREATE TABLE '" << name << "' (ind INTEGER(4), name TEXT, type TEXT, info TEXT);";
    exec(ss.str().c_str());
    ss.str("");
    ss << "INSERT INTO '" << name << "' VALUES(?,?,?,?);";
    insert_statement = prepare(ss.str());
}
    
void ProducerInfoTable::append(int index, const std::string & p_name, const std::string & p_type, const std::string & info){
    sqlite3_bind_int(insert_statement, 1, index);
    sqlite3_bind_text(insert_statement, 2, p_name.c_str(), p_name.size(), SQLITE_TRANSIENT);
    sqlite3_bind_text(insert_statement, 3, p_type.c_str(), p_type.size(), SQLITE_TRANSIENT);
    sqlite3_bind_text(insert_statement, 4, info.c_str(), info.size(), SQLITE_TRANSIENT);
    int res = sqlite3_step(insert_statement);
    sqlite3_reset(insert_statement);
    if (res != 101) {
        error(__FUNCTION__);//throws exception
    }
}

void RndInfoTable::create_table(){
    stringstream ss;
    ss << "CREATE TABLE '" << name << "' (runid INTEGER(4), seed INTEGER(8));";
    exec(ss.str().c_str());
    ss.str("");
    ss << "INSERT INTO '" << name << "' VALUES(?,?);";
    insert_statement = prepare(ss.str());
}

void RndInfoTable::append(const Run & run, long seed){
    sqlite3_bind_int(insert_statement, 1, run.get_runid());
    sqlite3_bind_int64(insert_statement, 2, seed);
    int res = sqlite3_step(insert_statement);
    sqlite3_reset(insert_statement);
    if (res != 101) {
        error(__FUNCTION__);//throws exception
    }
}

ParamTable::ParamTable(const std::string & name_, const theta::VarIdManager & vm, const theta::ParIds & ids):
    Table(name_), par_ids(ids){
    pid_names.reserve(par_ids.size());
    for(ParIds::const_iterator it=par_ids.begin(); it!=par_ids.end(); ++it){
        pid_names.push_back(vm.getName(*it));
    }
}

void ParamTable::create_table() {
    stringstream ss;
    ss << "CREATE TABLE '" << name << "' (runid INTEGER(4), eventid INTEGER(4)";
    for (vector<string>::const_iterator it = pid_names.begin(); it != pid_names.end(); ++it) {
        ss << ", '" << *it << "' DOUBLE";
    }
    ss << ");";
    exec(ss.str());
    //prepare insert statement:
    ss.str("");
    ss << "INSERT INTO " << name << " VALUES(?, ?";
    for (vector<string>::const_iterator it = pid_names.begin(); it != pid_names.end(); ++it) {
        ss << ", ?";
    }
    ss << ");";
    insert_statement = prepare(ss.str());
}

void ParamTable::append(const Run & run, const ParValues & values) {
    sqlite3_bind_int(insert_statement, 1, run.get_runid());
    sqlite3_bind_int(insert_statement, 2, run.get_eventid());
    int next_col = 3;
    for (ParIds::const_iterator it = par_ids.begin(); it != par_ids.end(); it++) {
        sqlite3_bind_double(insert_statement, next_col, values.get(*it));
        next_col++;
    }
    int res = sqlite3_step(insert_statement);
    sqlite3_reset( insert_statement);
    if (res != 101) {
        error(__FUNCTION__);//throws exception
    }
}
