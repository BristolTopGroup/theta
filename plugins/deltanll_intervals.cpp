#include "plugins/deltanll_intervals.hpp"
#include "interface/plugin.hpp"
#include "interface/run.hpp"
#include "interface/minimizer.hpp"
#include "interface/histogram.hpp"

#include <sstream>

using namespace theta;
using namespace std;
using namespace libconfig;

void deltanll_interval_table::create_table() {
    stringstream ss;
    ss << "CREATE TABLE '" << name << "' (runid INTEGER(4), eventid INTEGER(4), clevel DOUBLE, lower DOUBLE, upper DOUBLE);";
    exec(ss.str());
    ss.str("");
    ss << "INSERT INTO '" << name << "' VALUES(?,?,?,?,?);";
    insert_statement = prepare(ss.str());
}


void deltanll_interval_table::append(const Run & run, double clevel, double lower, double upper) {
    sqlite3_bind_int(insert_statement, 1, run.get_runid());
    sqlite3_bind_int(insert_statement, 2, run.get_eventid());
    sqlite3_bind_double(insert_statement, 3, clevel);
    sqlite3_bind_double(insert_statement, 4, lower);
    sqlite3_bind_double(insert_statement, 5, upper);
    int res = sqlite3_step(insert_statement);
    sqlite3_reset(insert_statement);
    if (res != 101) {
        error(__FUNCTION__);//throws exception
    }
}


void deltanll_intervals::produce(Run & run, const Data & data, const Model & model) {
    if(!table) table.connect(run.get_database());
    NLLikelihood nll = model.getNLLikelihood(data);

    MinimizationResult minres = minimizer->minimize(nll);

    //the "0" c.l. interval is the minimum:
    table.append(run, 0.0, minres.values.get(pid), minres.values.get(pid));

    //TODO: scan the parameter of interest ...
}

deltanll_intervals::deltanll_intervals(const theta::plugin::Configuration & cfg): Producer(cfg), table(get_name()){
    SettingWrapper s = cfg.setting;
    minimizer = theta::plugin::PluginManager<Minimizer>::build(theta::plugin::Configuration(cfg, s["minimizer"]));
        string par_name = s["parameter"];
        pid = cfg.vm->getParId(par_name);
        size_t ic = s["clevels"].size();
        if (ic == 0) {
            throw ConfigurationException("buildProducer[type=deltanll]: empty clevels.");
        }
        for (size_t i = 0; i < ic; i++) {
            clevels.push_back(s["clevels"][i]);
        }
    for(size_t i=0; i<clevels.size(); ++i){
        if(clevels[i] < 0.0) throw InvalidArgumentException("deltanll_intervals: clevel < 0 not allowed.");
        if(clevels[i] >= 1.0) throw InvalidArgumentException("deltanll_intervals: clevel >= 1.0 not allowed.");
        deltanll_levels[i] = utils::phi_inverse(1-(1-clevels[i])/2);
        deltanll_levels[i] *= deltanll_levels[i];
    }
}

//plugin is not ready: does not really produce intervals so far ...
//REGISTER_PLUGIN(deltanll_intervals)
