#include "run_plain.hpp"
#include "interface/histogram.hpp"

using namespace theta;
using namespace theta::plugin;
using namespace libconfig;
using namespace std;

void plain_run::run_impl() {
    for (eventid = 1; eventid <= n_event; eventid++) {
        if(stop_execution)break;
        log_event_start();
        ParValues values = m_pseudodata.sampleValues(rnd);
        data = m_pseudodata.samplePseudoData(rnd, values);
        params_table->append(*this, values);
        for (size_t j = 0; j < producers.size(); j++) {
            try {
                producers[j].produce(*this, data, m_producers);
            } catch (Exception & ex) {
                std::stringstream ss;
                ss << "Producer '" << producers[j].get_name() << "' failed: " << ex.message;
                logtable->append(*this, LogTable::error, ss.str());
            }
        }
        producer_table->add_row(*this);
        log_event_end();
        if(progress_listener) progress_listener->progress(eventid, n_event);
    }
}

plain_run::plain_run(const Configuration & cfg): Run(cfg){
}

REGISTER_PLUGIN(plain_run)
