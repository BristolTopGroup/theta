#include "plugins/postgresql_database.hpp"
#include "interface/plugin.hpp"
#include "interface/histogram.hpp"

#include <sstream>

#include <boost/filesystem.hpp>
#include <boost/scoped_array.hpp>

//to create a database cluster to use with this plugin as unpriviledged user:
//  initdb <dir>
//  #change unix_socket_directory in <dir>/postgresql.conf
//  #change port in  <dir>/postgresql.conf
//  postgres -D <dir>
//  createdb -h localhost -p ... <some db name>

using namespace std;
using namespace theta;

postgresql_database::postgresql_database(const plugin::Configuration & cfg) :
    conn(0), transaction_active(false){
    mode = "recreate";
    if(cfg.setting.exists("mode")){
        mode = static_cast<string>(cfg.setting["mode"]);
        if(mode!="recreate" and mode!="append"){
            throw ConfigurationException("invalid 'mode' setting given (allowed is only 'recreate' or 'append')");
        }
    }
    if(cfg.setting.exists("table_prefix")){
        table_prefix = static_cast<string>(cfg.setting["table_prefix"]);
        if(table_prefix!="")
        check_name(table_prefix);
    }
    std::string conninfo = cfg.setting["conninfo"];
    conn = PQconnectdb(conninfo.c_str());
    if(conn==0 || PQstatus(conn) != CONNECTION_OK){
        stringstream ss;
        const char * err = "<conn was null>";
        if(conn)  err = PQerrorMessage(conn);
        ss << "postgresql: could not connect to database: " << err;
        if(conn) PQfinish(conn);
        throw FatalException(ss.str());
    }
    beginTransaction();
}

void postgresql_database::close() {
    if (!conn)
        return;
    while(PGresult * pres = PQgetResult(conn)){
        PQclear(pres);
    }
    endTransaction();
    PQfinish(conn);
    conn = 0;
}

postgresql_database::~postgresql_database() {
    //Close, but do not throw on failure, just print it:
    try {
        close();
    } catch (Exception & e) {
        cerr << "Exception while closing database in destructor: " << e.message << endl << "Ingoring." << endl;
    }
}

void postgresql_database::beginTransaction() {
    if(!transaction_active){
        exec("BEGIN");
        transaction_active = true;
    }
}

void postgresql_database::endTransaction() {
    if (transaction_active)
        exec("COMMIT");
    transaction_active = false;
}

void postgresql_database::exec(const string & query) {
    PGresult * res = PQexec(conn, query.c_str());
    if(res==0 || PQresultStatus(res) != PGRES_COMMAND_OK){
        stringstream ss;
        ss << "error executing query '" << query << "': " << (res?PQresultErrorMessage(res):"<res was null>") << endl;
        cerr << "SQL error: " << ss.str() << endl;
        if(res!=0) PQclear(res);
        throw DatabaseException(ss.str());
    }
    if(res!=0) PQclear(res);
}

void postgresql_database::prepare(const string & sql, int ncol, const string & statement_name) {
    vector<Oid> pTypes(ncol, 0);
    PGresult * prep= PQprepare(conn, statement_name.c_str(), sql.c_str(), ncol, &pTypes[0]);
    if(PQresultStatus(prep) != PGRES_COMMAND_OK){
        PQclear(prep);
        error( __FUNCTION__);
    }
    PQclear(prep);
}

void postgresql_database::error(const string & functionName) {
    stringstream ss;
    ss << "Error in function " << functionName << ": Database said '" << PQerrorMessage(conn) << "'";
    throw DatabaseException(ss.str());
}

std::auto_ptr<Table> postgresql_database::create_table(const string & table_name){
    check_name(table_name);
    return std::auto_ptr<Table>(new postgresql_table(table_prefix + table_name, this));
}


postgresql_database::postgresql_table::postgresql_table(const string & name_, postgresql_database * db_) :
    name(name_), table_created(false), next_column(1), db(db_) {
        insert_statement = "insert_" + name;
}

std::auto_ptr<Column> postgresql_database::postgresql_table::add_column(const std::string & name, const data_type & type){
    if(table_created) throw IllegalStateException("Table::add_column called after table already created (via call to set_column / add_row).");
    column_names.push_back(name);
    column_types.push_back(type);
    column_definitions << (next_column > 1?", \"":"\"") << name << "\" ";
    switch(type){
        case typeDouble: column_definitions << "DOUBLE PRECISION"; break;
        case typeInt: column_definitions << "INTEGER"; break;
        case typeAutoIncrement: column_definitions << "SERIAL"; break;
        case typeString: column_definitions << "TEXT"; break;
        case typeHisto: column_definitions << "BYTEA"; break;
        default:
            throw InvalidArgumentException("Table::add_column: invalid type parameter given.");
    };
    std::auto_ptr<Column> result(new postgresql_column(next_column));
    column_content.resize(next_column);
    next_column++;
    return result;
}

namespace{
    int get_int(PGresult * res){
        char * count = PQgetvalue(res, 0, 0);
        if(count==0){
            PQclear(res);
            throw DatabaseException("get_int");
        }
        stringstream ss;
        ss << count;
        int result;
        ss >> result;
        return result;
    }
}

//check whether table already existing matches the own table definition.
// returns true if the table already exists and is compatble
// If a table exists but is not compatible, an Exception is thrown
// If the table does not exist, false is returned.
bool postgresql_database::postgresql_table::check_existing_table(){
    PGresult * res = PQexec(db->conn, ("select count(*) from pg_tables where tablename='" + name + "';").c_str());
    if(PQntuples(res)!=1){
        throw DatabaseException("check_existing_table: checking existence of table failed");
    }
    if(get_int(res)==0){
        PQclear(res);
        return false;
    }
    PQclear(res);
    
    db->prepare("select * from \"" + name + "\"", 0, "");
    res = PQdescribePrepared(db->conn, "");
    if(PQresultStatus(res) == PGRES_COMMAND_OK){
        if(PQnfields(res)!=column_names.size()){
            PQclear(res);
            throw DatabaseException("check_existing_table: table exists, but is not compatible");
        }
        for(int i=0; i<(int)column_names.size(); ++i){
            char * c_name = PQfname(res, i);
            if(c_name==0 || column_names[i] != c_name){
                PQclear(res);
                throw DatabaseException("check_existing_table: table exists, but is not compatible");
            }
        }
    }
    PQclear(res);
    return true;
}

void postgresql_database::postgresql_table::create_table(){
    bool create = false;
    if(db->mode=="recreate"){
        db->exec("DROP TABLE IF EXISTS \"" + name + "\";");
        create = true;
    }
    else{//append mode
        create = not check_existing_table();
    }
    
    stringstream ss;
    // NOTE: potential race condition: in the meantime, someone else could have created
    // the table we are about to create. Currently, "create table" will fail, so theta
    // will exit with an error. This is not strictly necessary as the created
    // table might have been compatible, but it is Ok as it should only happen very rarely ...
    if(create){
        string col_def = column_definitions.str();
        ss << "CREATE TABLE \"" << name << "\" (" << col_def << ") WITH (fillfactor=100, toast.autovacuum_enabled=false);";
        db->exec(ss.str());
    }
    
    db->endTransaction();
    db->beginTransaction();
    
    ss.str("");
    ss << "INSERT INTO \"" << name << "\" VALUES(";
    for(int i=1; i<next_column; ++i){
        if(i > 1) ss << ", $" << i;
        else ss << "$" << i;
    }
    ss << ");";
    db->prepare(ss.str(), next_column-1, insert_statement);
    table_created = true;
}

//create the table if it is empty to ensure that all tables have been created
// even if there are no entries
postgresql_database::postgresql_table::~postgresql_table(){
    if(not table_created) create_table();
}

void postgresql_database::postgresql_table::set_column(const Column & c, double d){
    char * & content = column_content[static_cast<const postgresql_column&>(c).column_index - 1];
    if(content==0){
        content = new char[32];
    }
    sprintf(content, "%.17e", d);
}

void postgresql_database::postgresql_table::set_column(const Column & c, int i){
    char * & content = column_content[static_cast<const postgresql_column&>(c).column_index - 1];
    if(content==0){
        content = new char[32];
    }
    sprintf(content, "%d", i);
}

void postgresql_database::postgresql_table::set_column(const Column & c, const std::string & s){
    char * & content = column_content[static_cast<const postgresql_column&>(c).column_index - 1];
    delete[] content;
    content = new char[s.size() + 1];
    copy(s.begin(), s.end(), content);
    content[s.size()] = '\0';
}

void postgresql_database::postgresql_table::set_column(const Column & c, const theta::Histogram & h){
    if(not table_created) create_table();
    //including overflow and underflow, we have nbins+2 bins. Encoding the range with the first
    // two, we have nbins+4 double to save.
    boost::scoped_array<double> blob_data(new double[h.get_nbins()+4]);
    blob_data[0] = h.get_xmin();
    blob_data[1] = h.get_xmax();
    std::copy(h.getData(), h.getData() + h.get_nbins()+2, &blob_data[2]);
    size_t nbytes = sizeof(double) * (h.get_nbins() + 4);

    //psql-specific escape:
    size_t to_length;
    unsigned char * escaped_blob_data = PQescapeByteaConn(db->conn, reinterpret_cast<unsigned char*>(&blob_data[0]), nbytes, &to_length);

    //copy to column buffer:
    char * & content = column_content[static_cast<const postgresql_column&>(c).column_index - 1];
    delete[] content;
    content = new char[to_length + 1];
    copy(escaped_blob_data, escaped_blob_data + to_length, content);
    content[to_length] = '\0';
    PQfreemem(escaped_blob_data);
}

void postgresql_database::postgresql_table::add_row(){
    if(not table_created) create_table();
    //make sure previous statement has executed completely:
    while(PGresult * pres = PQgetResult(db->conn)){
        PQclear(pres);
    }
    //send next insertion asynchronously ...
    PQsendQueryPrepared(db->conn, insert_statement.c_str(), next_column-1, &(column_content[0]), 0, 0, 1);
}



int postgresql_database::postgresql_table::add_row_autoinc(const theta::Column & c){
    if(not table_created) create_table();
    while(PGresult * pres = PQgetResult(db->conn)){
        PQclear(pres);
    }
    int index = static_cast<const postgresql_column&>(c).column_index;
    stringstream ss;
    ss << "select nextval('" << name << "_" << column_names[index-1] << "_seq" << "');";
    PGresult * res = PQexec(db->conn, ss.str().c_str());
    int nextval = get_int(res);
    PQclear(res);
    set_column(c, nextval);
    add_row();
    return nextval;
}

REGISTER_PLUGIN(postgresql_database)
