#include "run_plain.hpp"

using namespace theta;
using namespace theta::plugin;
using namespace libconfig;
using namespace std;

void PlainRun::run_impl() {
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
    }
}


std::auto_ptr<Run> PlainRunFactory::build(ConfigurationContext & ctx) const {
    const Setting & s = ctx.setting;
    std::auto_ptr<PlainRun> result;
    //runid defaults to 1:
    int runid = 1;
    if(s.lookupValue(static_cast<const char*>("run-id"), runid)){
        ctx.rec.markAsUsed(s["run-id"]);
    }
    string outfile = s["result-file"];
    ctx.rec.markAsUsed(s["result-file"]);
    int seed = -1;
    if(s.lookupValue(static_cast<const char*>("seed"), seed)){
        ctx.rec.markAsUsed(s["seed"]);
    }
    if (seed < 0) {//auto:
        seed = time(0);
    }

    const int n_event = s["n-events"];
    ctx.rec.markAsUsed(s["n-events"]);
    
    //build Models:
    boost::shared_ptr<Model> model;
    boost::shared_ptr<Model> m_pseudodata, m_producers;
    if (s.exists("model")) {
        string model_path = s["model"];
        ConfigurationContext context(ctx, ctx.rootsetting[model_path]);
        model = ModelFactory::buildModel(context);
        m_pseudodata = model;
        m_producers = model;
        ctx.rec.markAsUsed(s["model"]);
    }
    if (s.exists("model-pseusodata")) {
        string model_path = s["model-pseudodata"];
        ConfigurationContext context(ctx, ctx.rootsetting[model_path]);
        m_pseudodata = ModelFactory::buildModel(context);
        ctx.rec.markAsUsed(s["model-pseudodata"]);
    }
    if (s.exists("model-producers")) {
        string model_path = s["model-producers"];
        ConfigurationContext context(ctx, ctx.rootsetting[model_path]);
        m_producers = ModelFactory::buildModel(context);
        ctx.rec.markAsUsed(s["model-producers"]);
    }
    //create result, depending on the mode:
    result.reset(new PlainRun(seed, *m_pseudodata, *m_producers, outfile, runid, n_event));
    //add the producers:
    int n_p = s["producers"].getLength();
    if (n_p == 0)
        throw ConfigurationException("buildRun: no producers in run specified!");
    for (int i = 0; i < n_p; i++) {
        string prod_path = s["producers"][i];
        ConfigurationContext ctx2(ctx, ctx.rootsetting[prod_path]);
        std::auto_ptr<Producer> p = PluginManager<ProducerFactory>::get_instance()->build(ctx2);
        result->addProducer(p);
        //should transfer ownership:
        assert(p.get()==0);
    }
    ctx.rec.markAsUsed(s["producers"]);
    return std::auto_ptr<Run>(result.release());
}

REGISTER_FACTORY(PlainRunFactory)
