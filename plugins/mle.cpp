#include "plugins/mle.hpp"
#include "interface/plugin.hpp"

#include <sstream>

using namespace theta;
using namespace std;
using namespace libconfig;

MLETable::MLETable(const std::string & name_, const theta::VarIdManager & vm, const theta::ParIds & ids): Table(name_), save_ids(ids){
    pid_names.reserve(ids.size());
    for(ParIds::const_iterator it=ids.begin(); it!=ids.end(); ++it){
        pid_names.push_back(vm.getName(*it));
    }
}

void MLETable::create_table() {
    stringstream ss;
    ss << "CREATE TABLE '" << name << "' (runid INTEGER(4), eventid INTEGER(4), nll DOUBLE";
    for (vector<string>::const_iterator it = pid_names.begin(); it != pid_names.end(); ++it) {
        ss << ", '" << *it << "' DOUBLE, '" << *it << "_error' DOUBLE";
    }
    ss << ");";
    exec(ss.str());
    ss.str("");
    ss << "INSERT INTO '" << name << "' VALUES(?,?,?";
    for (vector<string>::const_iterator it = pid_names.begin(); it != pid_names.end(); ++it) {
        ss << ",?,?";
    }
    ss << ");";
    insert_statement = prepare(ss.str());
}

void MLETable::append(const Run & run, double nll, const ParValues & values, const ParValues & errors) {
    sqlite3_bind_int(insert_statement, 1, run.get_runid());
    sqlite3_bind_int(insert_statement, 2, run.get_eventid());
    sqlite3_bind_double(insert_statement, 3, nll);
    int i=4;
    for(ParIds::const_iterator it=save_ids.begin(); it!=save_ids.end(); ++it){
        sqlite3_bind_double(insert_statement, i, values.get(*it));
        i++;
        sqlite3_bind_double(insert_statement, i, errors.get(*it));
        i++;
    }
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

MLEProducer::MLEProducer(const VarIdManager & vm, const ParIds & save_ids, std::auto_ptr<Minimizer> & min, const string & name_) :
    Producer(name_), minimizer(min), table(name_, vm, save_ids) {
}

void MLEProducer::produce(Run & run, const Data & data, const Model & model) {
    if(!table) table.connect(run.get_database());
    NLLikelihood nll = model.getNLLikelihood(data);
    boost::shared_ptr<Function> function_ptr(&nll, noop_deleter<Function>());
    MinimizationResult minres = minimizer->minimize(function_ptr);
    table.append(run, minres.fval, minres.values, minres.errors_plus);
}

std::auto_ptr<Producer> MLEProducerFactory::build(theta::plugin::ConfigurationContext & ctx) const {
    const Setting & s = ctx.setting;
    string minimizer_path = s["minimizer"];
    Setting & minimizer_setting = ctx.rootsetting[minimizer_path];
    theta::plugin::ConfigurationContext ctx_min(ctx, minimizer_setting);
    std::auto_ptr<Minimizer> minimizer = theta::plugin::PluginManager<theta::plugin::MinimizerFactory>::get_instance()->build(ctx_min);
    ParIds save_ids;
    int n_parameters = s["parameters"].getLength();
    for (int i = 0; i < n_parameters; i++) {
        string par_name = s["parameters"][i];
        save_ids.insert(ctx.vm->getParId(par_name));
    }
    ctx.rec.markAsUsed(s["parameters"]);
    return std::auto_ptr<Producer>(new MLEProducer(*ctx.vm, save_ids, minimizer, s.getName()));
}

REGISTER_FACTORY(MLEProducerFactory)
