#include "plugins/deltanll_intervals.hpp"
#include "interface/plugin.hpp"
#include "interface/run.hpp"
#include "interface/minimizer.hpp"
#include <sstream>

using namespace theta;
using namespace std;
using namespace libconfig;

void DeltaNllIntervalTable::create_table() {
    stringstream ss;
    ss << "CREATE TABLE '" << name << "' (runid INTEGER(4), eventid INTEGER(4), clevel DOUBLE, lower DOUBLE, upper DOUBLE);";
    exec(ss.str());
    ss.str("");
    ss << "INSERT INTO '" << name << "' VALUES(?,?,?,?,?);";
    insert_statement = prepare(ss.str());
}


void DeltaNllIntervalTable::append(const Run & run, double clevel, double lower, double upper) {
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

deltanll_intervals::deltanll_intervals(theta::plugin::Configuration & ctx): Producer(ctx), table(getName()){
    const Setting & s = ctx.setting;
    std::auto_ptr<Producer> result;
    string minimizer_path = s["minimizer"];
    Setting & minimizer_setting = ctx.rootsetting[minimizer_path];
    theta::plugin::Configuration ctx_min(ctx, minimizer_setting);
    minimizer = theta::plugin::PluginManager<Minimizer>::build(ctx_min);
        string par_name = s["parameter"];
        ctx.rec.markAsUsed(s["parameter"]);
        pid = ctx.vm->getParId(par_name);
        int ic = s["clevels"].getLength();
        if (ic == 0) {
            throw ConfigurationException("buildProducer[type=deltanll]: empty clevels.");
        }
        for (int i = 0; i < ic; i++) {
            clevels.push_back(s["clevels"][i]);
        }
        ctx.rec.markAsUsed(s["clevels"]);
    for(size_t i=0; i<clevels.size(); ++i){
        if(clevels[i] < 0.0) throw InvalidArgumentException("deltanll_intervals: clevel < 0 not allowed.");
        if(clevels[i] >= 1.0) throw InvalidArgumentException("deltanll_intervals: clevel >= 1.0 not allowed.");
        deltanll_levels[i] = utils::phi_inverse(1-(1-clevels[i])/2);
        deltanll_levels[i] *= deltanll_levels[i];
    }
}

//plugin is not ready: does not really produce intervals so far ...
REGISTER_PLUGIN(deltanll_intervals)
