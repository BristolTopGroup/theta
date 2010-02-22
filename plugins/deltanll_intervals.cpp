#include "plugins/deltanll_intervals.hpp"
#include "interface/plugin.hpp"

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

DeltaNLLIntervalProducer::DeltaNLLIntervalProducer(const ParId & pid_, const vector<double> & clevels_,
        std::auto_ptr<Minimizer> & min, const string & name_) :
    Producer(name_), pid(pid_), clevels(clevels_), minimizer(min), deltanll_levels(clevels.size()), table(name_) {
    //auto_ptr has "transfer" semantics:
    assert(min.get()==0 && minimizer.get()!=0);
    //fill deltanll_levels:
    for(size_t i=0; i<clevels.size(); ++i){
        if(clevels[i] < 0.0) throw InvalidArgumentException("DeltaNLLIntervalProducer: clevel < 0 not allowed.");
        if(clevels[i] >= 1.0) throw InvalidArgumentException("DeltaNLLIntervalProducer: clevel >= 1.0 not allowed.");
        deltanll_levels[i] = utils::phi_inverse(1-(1-clevels[i])/2);
        deltanll_levels[i] *= deltanll_levels[i];
        //cout << "deltanll for " << clevels[i] << " is " << deltanll_levels[i] << endl;
    }
}

template<typename T>
struct noop_deleter{
    void operator()(T* t){}
};

void DeltaNLLIntervalProducer::produce(Run & run, const Data & data, const Model & model) {
    if(!table) table.connect(run.get_database());
    NLLikelihood nll = model.getNLLikelihood(data);

    //note that nll ius stack-allocated, so we need to tell the shared_ptr
    // that nll should not be deleted if the use count drops to zero:
    boost::shared_ptr<Function> function_ptr(&nll, noop_deleter<Function>());
    MinimizationResult minres = minimizer->minimize(function_ptr);

    //the "0" c.l. interval is the minimum:
    table.append(run, 0.0, minres.values.get(pid), minres.values.get(pid));

    //TODO: scan the parameter of interest ...
}

std::auto_ptr<Producer> DeltaNLLProducerFactory::build(theta::plugin::ConfigurationContext & ctx)const{
    const Setting & s = ctx.setting;
    std::auto_ptr<Producer> result;
    string minimizer_path = s["minimizer"];
    Setting & minimizer_setting = ctx.rootsetting[minimizer_path];
    theta::plugin::ConfigurationContext ctx_min(ctx, minimizer_setting);
    std::auto_ptr<Minimizer> minimizer = theta::plugin::PluginManager<theta::plugin::MinimizerFactory>::get_instance()->build(ctx_min);
        string par_name = s["parameter"];
        ctx.rec.markAsUsed(s["parameter"]);
        ParId pid = ctx.vm->getParId(par_name);
        vector<double> clevels;
        int ic = s["clevels"].getLength();
        if (ic == 0) {
            throw ConfigurationException("buildProducer[type=deltanll]: empty clevels.");
        }
        for (int i = 0; i < ic; i++) {
            clevels.push_back(s["clevels"][i]);
        }
        ctx.rec.markAsUsed(s["clevels"]);
        result.reset(new DeltaNLLIntervalProducer(pid, clevels, minimizer, s.getName()));
    return result;
}

//plugin is not ready: does not really produce intervals so far ...
REGISTER_FACTORY(DeltaNLLProducerFactory)

