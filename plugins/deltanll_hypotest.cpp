#include "plugins/deltanll_hypotest.hpp"
#include "interface/plugin.hpp"

#include <sstream>

using namespace theta;
using namespace std;
using namespace libconfig;

void DeltaNLLHypotestTable::create_table() {
    stringstream ss;
    ss << "CREATE TABLE '" << name << "' (runid INTEGER(4), eventid INTEGER(4), nll_sb DOUBLE, nll_b DOUBLE);";
    exec(ss.str());
    ss.str("");
    ss << "INSERT INTO '" << name << "' VALUES(?,?,?,?);";
    insert_statement = prepare(ss.str());
}

void DeltaNLLHypotestTable::append(const Run & run, double nll_sb, double nll_b) {
    sqlite3_bind_int(insert_statement, 1, run.get_runid());
    sqlite3_bind_int(insert_statement, 2, run.get_eventid());
    sqlite3_bind_double(insert_statement, 3, nll_sb);
    sqlite3_bind_double(insert_statement, 4, nll_b);
    int res = sqlite3_step(insert_statement);
    sqlite3_reset(insert_statement);
    if (res != 101) {
        error(__FUNCTION__);//throws exception
    }
}

template<typename T>
struct noop_deleter{
    void operator()(T* t){}
};

DeltaNLLHypotestProducer::DeltaNLLHypotestProducer(const ParValues & s_plus_b_, const ParValues & b_only_, std::auto_ptr<Minimizer> & min, const string & name_) :
    Producer(name_), s_plus_b(s_plus_b_), b_only(b_only_), minimizer(min), table(name_) {
    par_ids_constraints = s_plus_b.getAllParIds();
    ParIds par_ids_b = b_only.getAllParIds();
    for(ParIds::const_iterator it=par_ids_b.begin(); it!=par_ids_b.end(); ++it){
        par_ids_constraints.insert(*it);
    }
}

void DeltaNLLHypotestProducer::produce(Run & run, const Data & data, const Model & model) {
    if(!table) table.connect(run.get_database());
    NLLikelihood nll = model.getNLLikelihood(data);
    //see above for this type of noop_deleter construction
    boost::shared_ptr<Function> function_ptr(&nll, noop_deleter<Function>());
    double nll_sb, nll_b;
    nll_sb = NAN;
    nll_b = NAN;
    //a. calculate s plus b:
    //apply s_plus_b constraints and take back any others:
    for(ParIds::const_iterator it = par_ids_constraints.begin(); it!=par_ids_constraints.end(); ++it){
        if(s_plus_b.contains(*it)){
            minimizer->override_range(*it, s_plus_b.get(*it), s_plus_b.get(*it));
        }else{
            minimizer->reset_range_override(*it);
        }
    }
    try{
        MinimizationResult minres = minimizer->minimize(function_ptr);
        nll_sb = minres.fval;
    }
    catch(Exception & ex){
        cerr << "minres(1): "<< ex.message << endl;
        exit(1);
    }
    
    //b. calculate b only:
    //apply b contraints:
    for(ParIds::const_iterator it = par_ids_constraints.begin(); it!=par_ids_constraints.end(); ++it){
        if(b_only.contains(*it))
            minimizer->override_range(*it, b_only.get(*it), b_only.get(*it));
        else
            minimizer->reset_range_override(*it);
    }
    MinimizationResult minres = minimizer->minimize(function_ptr);
    nll_b = minres.fval;
    table.append(run, nll_sb, nll_b);
}


std::auto_ptr<Producer> DeltaNLLHypotestProducerFactory::build(theta::plugin::ConfigurationContext & ctx) const {
    const Setting & s = ctx.setting;
    string minimizer_path = s["minimizer"];
    Setting & minimizer_setting = ctx.rootsetting[minimizer_path];
    theta::plugin::ConfigurationContext ctx_min(ctx, minimizer_setting);
    std::auto_ptr<Minimizer> minimizer = theta::plugin::PluginManager<theta::plugin::MinimizerFactory>::get_instance()->build(ctx_min);
    ParValues s_plus_b;
    ParValues b_only;
    int sb_i = s["signal-plus-background"].getLength();
    for (int i = 0; i < sb_i; i++) {
        string par_name = s["signal-plus-background"][i].getName();
        double par_value = s["signal-plus-background"][i];
        s_plus_b.set(ctx.vm->getParId(par_name), par_value);
    }
    ctx.rec.markAsUsed(s["signal-plus-background"]);
    int b_i = s["background-only"].getLength();
    for (int i = 0; i < b_i; i++) {
        string par_name = s["background-only"][i].getName();
        double par_value = s["background-only"][i];
        b_only.set(ctx.vm->getParId(par_name), par_value);
    }
    ctx.rec.markAsUsed(s["background-only"]);
    return std::auto_ptr<Producer>(new DeltaNLLHypotestProducer(s_plus_b, b_only, minimizer, s.getName()));
}

REGISTER_FACTORY(DeltaNLLHypotestProducerFactory)
