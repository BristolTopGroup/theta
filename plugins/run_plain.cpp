#include "run_plain.hpp"

using namespace theta;
using namespace theta::plugin;
using namespace libconfig;
using namespace std;

void plain_run::run_impl() {
    for (eventid = 1; eventid <= n_event; eventid++) {
        log_event_start();
        fill_data();
        for (size_t j = 0; j < producers.size(); j++) {
            try {
                producers[j].produce(*this, data, m_producers);
            } catch (Exception & ex) {
                std::stringstream ss;
                ss << "Producer '" << producers[j].getName() << "' failed: " << ex.message;
                logtable->append(*this, database::severity::error, ss.str());
            }
        }
        log_event_end();
        if(progress_listener)progress_listener->progress(eventid, n_event);
    }
}


plain_run::plain_run(Configuration & cfg): Run(cfg){
}

REGISTER_PLUGIN(plain_run)

