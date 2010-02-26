#include "scan_run.hpp"

using namespace theta;
using namespace theta::plugin;
using namespace libconfig;
using namespace std;

void scan_run::run_impl() {
    for(runid=1; runid<=(int)scan_values.size(); ++runid){
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
        const ObsIds & obs_ids = data.getObservables();
                std::map<theta::ObsId, double> n_data;
            for(ObsIds::const_iterator obsit=obs_ids.begin(); obsit!=obs_ids.end(); obsit++){
                n_data[*obsit] = data.getData(*obsit).get_sum_of_bincontents();
        }
        params_table->append(*this, values, n_data);
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
        if(progress_listener)progress_listener->progress(n_event * (runid-1) + eventid, n_event * scan_values.size());
    }
    }
}


scan_run::scan_run(Configuration & cfg): Run(cfg){
    string parameter = cfg.setting["scan-parameter"];
    int n = cfg.setting["scan-parameter-values"].getLength();
    pid = cfg.vm->getParId(parameter);
    for(int i=0; i<n; ++i){
       scan_values.push_back(cfg.setting["scan-parameter-values"][i]);
    }
    scan_parameter_fixed = cfg.setting["scan-parameter-fixed"];
}

REGISTER_PLUGIN(scan_run)

