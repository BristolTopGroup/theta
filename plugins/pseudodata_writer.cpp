#include "plugins/pseudodata_writer.hpp"
#include "interface/plugin.hpp"
#include "interface/run.hpp"
#include "interface/phys.hpp"
#include "interface/histogram.hpp"
#include <sstream>

using namespace theta;
using namespace std;
using namespace libconfig;

void PseudodataTable::create_table() {
    stringstream ss;
    ss << "CREATE TABLE '" << name << "' (runid INTEGER(4), eventid INTEGER(4) ";
    size_t i=0;
    for(ObsIds::const_iterator it=observables.begin(); it!=observables.end(); ++it, ++i){
        ss << ", 'n_events_" << observable_names[i] << "' DOUBLE";
        if(write_data){
            ss << ", 'data_" << observable_names[i] << "' BLOB";
        }
    }
    ss << ");";
    exec(ss.str());
    ss.str("");
    ss << "INSERT INTO '" << name << "' VALUES(?,?";
    for(ObsIds::const_iterator it=observables.begin(); it!=observables.end(); ++it){
        ss << ", ?";
        if(write_data){
            ss << ", ?";
        }
    }
    ss << ");";
    insert_statement = prepare(ss.str());
}

void PseudodataTable::append(const theta::Run & run, const Data & data) {
    sqlite3_bind_int(insert_statement, 1, run.get_runid());
    sqlite3_bind_int(insert_statement, 2, run.get_eventid());
    int next_col = 3;
    for(ObsIds::const_iterator it=observables.begin(); it!=observables.end(); ++it){
        const Histogram & h = data.getData(*it);
        double n_event = h.get_sum_of_bincontents();
        sqlite3_bind_double(insert_statement, next_col, n_event);
        ++next_col;
        if(write_data){
            const double * double_data = h.getData();
            sqlite3_bind_blob(insert_statement, next_col, double_data, sizeof(double)*(h.get_nbins() + 2), SQLITE_TRANSIENT);
            ++next_col;
        }
    }
    int res = sqlite3_step(insert_statement);
    sqlite3_reset(insert_statement);
    if (res != 101) {
        error(__FUNCTION__);//throws exception
    }
}

void pseudodata_writer::produce(Run & run, const Data & data, const Model & model) {
    if(!table) table.connect(run.get_database());
    table.append(run, data);
}

pseudodata_writer::pseudodata_writer(const theta::plugin::Configuration & cfg): Producer(cfg), table(get_name()){
    size_t n = cfg.setting["observables"].size();
    table.observable_names.reserve(n);
    for(size_t i=0; i<n; i++){
        table.observable_names.push_back(cfg.setting["observables"][i]);
        table.observables.insert(cfg.vm->getObsId(table.observable_names.back()));
    }
    table.write_data = cfg.setting["write-data"];
}

REGISTER_PLUGIN(pseudodata_writer)
