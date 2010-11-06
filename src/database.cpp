#include <string>
#include <sstream>

#include <boost/date_time/local_time/local_time.hpp>

#include "interface/database.hpp"
#include "interface/run.hpp"
#include "interface/histogram.hpp"

using namespace std;
using namespace theta;

void Database::check_name(const string & column_name) {
    if (column_name.size() == 0)
        throw DatabaseException("Database::check_name: name was empty.");
    if (not ((column_name[0] >= 'A' && column_name[0] <= 'Z') or (column_name[0] >= 'a' && column_name[0] <= 'z'))) {
        stringstream ss;
        ss << "Database::check_name: '" << column_name << "' does not start with a letter as is required.";
        throw DatabaseException(ss.str());
    }
    for (size_t i = 1; i < column_name.size(); i++) {
        if (column_name[i] >= 'A' && column_name[i] <= 'Z')
            continue;
        if (column_name[i] >= 'a' && column_name[i] <= 'z')
            continue;
        if (column_name[i] >= '0' && column_name[i] <= '9')
            continue;
        if (column_name[i] == '_')
            continue;
        stringstream ss;
        ss << "Database::check_name: name '" << column_name << "' invalid at letter " << i;
        throw DatabaseException(ss.str());
    }
}

Table::~Table(){}

Column::~Column(){}

/* ProductsTable */
ProductsTable::ProductsTable(std::auto_ptr<Table> & table_): table(table_){
    c_runid = table->add_column("runid", Table::typeInt);
    c_eventid = table->add_column("eventid", Table::typeInt);
}

std::auto_ptr<Column> ProductsTable::add_column(const theta::plugin::ProductsTableWriter & tw, const std::string & column_name, const Table::data_type & type){
    std::string new_name = tw.getName() + "__" + column_name;
    return table->add_column(new_name, type);
}

void ProductsTable::add_row(const Run & run){
    table->set_column(*c_runid, run.get_runid());
    table->set_column(*c_eventid, run.get_eventid());
    table->add_row();
}

/* LogTable */
LogTable::LogTable(std::auto_ptr<Table> & table_): level(info), table(table_){
   for(int i=0; i<4; ++i){
       n_messages[i]=0;
   }
   c_runid = table->add_column("runid", Table::typeInt);
   c_eventid = table->add_column("eventid", Table::typeInt);
   c_severity = table->add_column("severity", Table::typeInt);
   c_message = table->add_column("message", Table::typeString);
   c_time = table->add_column("time", Table::typeDouble);
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
    table->set_column(*c_runid, run.get_runid());
    table->set_column(*c_eventid, run.get_eventid());
    table->set_column(*c_severity, s);
    table->set_column(*c_message, message);
    using namespace boost::posix_time;
    using namespace boost::gregorian;
    ptime t(microsec_clock::universal_time());
    time_duration td = t - ptime(date(1970, 1, 1));
    double time = td.total_microseconds() / 1000000.0;
    table->set_column(*c_time, time);
    table->add_row();
}

// ProducerInfoTable
ProducerInfoTable::ProducerInfoTable(std::auto_ptr<Table> & table_): table(table_){
    c_ind = table->add_column("ind", Table::typeInt);
    c_name = table->add_column("name", Table::typeString);
    c_type = table->add_column("type", Table::typeString);
}
    
void ProducerInfoTable::append(int index, const std::string & p_name, const std::string & p_type){
    table->set_column(*c_ind, index);
    table->set_column(*c_name, p_name);
    table->set_column(*c_type, p_type);
    table->add_row();
}

//RndInfoTable
RndInfoTable::RndInfoTable(std::auto_ptr<Table> & table_): table(table_){
    table->set_autoinc_column("runid");
    c_seed = table->add_column("seed", Table::typeInt);
}

int RndInfoTable::append(int seed){
    table->set_column(*c_seed, seed);
    return table->add_row();
}
