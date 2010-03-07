#include "plugins/pseudodata_writer.hpp"
#include "interface/plugin.hpp"
#include "interface/run.hpp"
#include "interface/phys.hpp"
#include "interface/histogram.hpp"
#include <sstream>

using namespace theta;
using namespace std;
using namespace libconfig;

void pseudodata_writer::define_table(){
    for(size_t i=0; i<observables.size(); ++i){
        n_events_columns.push_back(table->add_column(*this, "n_events_" + vm->getName(observables[i]), ProducerTable::typeDouble));
        if(write_data)
            data_columns.push_back(table->add_column(*this, "data_" + vm->getName(observables[i]), ProducerTable::typeBlob));
    }
}

void pseudodata_writer::produce(Run & run, const Data & data, const Model & model) {
    for(size_t i=0; i<observables.size(); ++i){
        const Histogram & h = data.getData(observables[i]);
        double n_event = h.get_sum_of_bincontents();
        table->set_column(n_events_columns[i], n_event);
        if(write_data){
            const double * double_data = h.getData();
            table->set_column(data_columns[i], double_data, sizeof(double) * (h.get_nbins() + 2));
        }
    }
}

pseudodata_writer::pseudodata_writer(const theta::plugin::Configuration & cfg): Producer(cfg), vm(cfg.vm){
    size_t n = cfg.setting["observables"].size();
    observables.reserve(n);
    for(size_t i=0; i<n; i++){
        observables.push_back(cfg.vm->getObsId(cfg.setting["observables"][i]));
    }
    write_data = cfg.setting["write-data"];
}

REGISTER_PLUGIN(pseudodata_writer)
