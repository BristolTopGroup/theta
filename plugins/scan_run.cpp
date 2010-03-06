#include "scan_run.hpp"
#include "interface/histogram.hpp"

using namespace theta;
using namespace theta::plugin;
using namespace libconfig;
using namespace std;

void scan_run::run_impl() {
    for(runid=1; runid<=(int)scan_values.size(); ++runid){
        if(stop_execution)break;
        const double scan_value = scan_values[runid-1];
        if(scan_parameter_fixed){
            vm->set_range_default(pid, scan_value, scan_value, scan_value);
        }
        for (eventid = 1; eventid <= n_event; eventid++) {
            log_event_start();
            //like fill_data(), but set a parameter by hand:
            ParValues values = m_pseudodata.sampleValues(rnd);
            values.set(pid, scan_value);
            data = m_pseudodata.samplePseudoData(rnd, values);
            params_table->append(*this, values);
            for (size_t j = 0; j < producers.size(); j++) {
                try {
                    producers[j].produce(*this, data, m_producers);
                } catch (Exception & ex) {
                    std::stringstream ss;
                    ss << "Producer '" << producers[j].get_name() << "' failed: " << ex.message;
                    logtable->append(*this, database::severity::error, ss.str());
                }
            }
            log_event_end();
            if(progress_listener)progress_listener->progress(n_event * (runid-1) + eventid, n_event * scan_values.size());
        }
    }
}


scan_run::scan_run(const Configuration & cfg): Run(cfg){
    string parameter = cfg.setting["scan-parameter"];
    size_t n = cfg.setting["scan-parameter-values"].size();
    if(n==0){
        throw ConfigurationException("scan-parameter-values is empty (or not a list/array)");
    }
    pid = cfg.vm->getParId(parameter);
    for(size_t i=0; i<n; ++i){
       scan_values.push_back(cfg.setting["scan-parameter-values"][i]);
    }
    scan_parameter_fixed = cfg.setting["scan-parameter-fixed"];
}

REGISTER_PLUGIN(scan_run)

