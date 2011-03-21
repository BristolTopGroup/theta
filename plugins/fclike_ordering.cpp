#include "plugins/fclike_ordering.hpp"
#include "interface/model.hpp"
#include "interface/database.hpp"


using namespace theta::plugin;
using namespace theta;
using namespace std;




class SaveDoubleProducts: public ProductsSink{
private:
    class MemSaveProductsColumn: public theta::Column{
        public:
            MemSaveProductsColumn(const string & name_): name(name_){}
            string name;
    };
    map<string, double> double_data;
public:
    virtual std::auto_ptr<Column> declare_product(const ProductsSource & source, const std::string & product_name, const data_type & type){
        return std::auto_ptr<Column>(new MemSaveProductsColumn(product_name));
    }
    
    virtual void set_product(const Column & c, double d){
        double_data[static_cast<const MemSaveProductsColumn &>(c).name] = d;
    }
    virtual void set_product(const Column & c, int i){
    }
    virtual void set_product(const Column & c, const std::string & s){
    }
    virtual void set_product(const Column & c, const Histogram & h){
    }
    
    double get_product(const std::string & name){
        map<string, double>::const_iterator it = double_data.find(name);
        if(it==double_data.end()) throw NotFoundException("unknown product '" + name + "'");
        return it->second;
    }
};


fclike_ordering::fclike_ordering(const Configuration & cfg): poi(cfg.vm->getParId(cfg.setting["parameter_of_interest"])){
    save_double_products.reset(new SaveDoubleProducts());
    cfg.pm->set<ProductsSink>("default", save_double_products);
    database = PluginManager<Database>::instance().build(Configuration(cfg, cfg.setting["output_database"]));
   
    std::auto_ptr<Table> rndinfo_table_underlying = database->create_table("rndinfo");
    boost::shared_ptr<RndInfoTable> rndinfo_table(new RndInfoTable(rndinfo_table_underlying));
    cfg.pm->set("default", rndinfo_table);
   
    model = PluginManager<Model>::instance().build(Configuration(cfg, cfg.setting["model"]));

    poi_min = cfg.setting["parameter_range"][0];
    poi_max = cfg.setting["parameter_range"][1];
    if(poi_min >= poi_max) throw ConfigurationException("parameter_range is not a proper range: first entry in range must be smaller than second");
    n_poi_scan = cfg.setting["n_parameter_scan"];
    if(n_poi_scan < 2) throw ConfigurationException("n_parameter_scan must be >= 2");
   
    ts_producer = PluginManager<Producer>::instance().build(Configuration(cfg, cfg.setting["ts_producer"]));
    ts_name = static_cast<string>(cfg.setting["ts_name"]);
}

void fclike_ordering::run(){
    std::auto_ptr<Table> ordering_table = database->create_table("ordering");
    std::auto_ptr<Column> c_truth = ordering_table->add_column("truth", typeDouble);
    std::auto_ptr<Column> c_ts = ordering_table->add_column("ts", typeDouble);
    std::auto_ptr<Column> c_ordering = ordering_table->add_column("ordering", typeDouble);
    
    ParValues mode;
    std::map<theta::ParId, std::pair<double, double> > support;
    DistributionUtils::fillModeSupport(mode, support, model->get_parameter_distribution());
    double poi_step = (poi_max - poi_min) / (n_poi_scan - 1);
    Data data;
    for(int i=0; i<n_poi_scan; ++i){
        double poi_value1 = poi_min + i * poi_step;
        mode.set(poi, poi_value1);
        model->get_prediction(data, mode);
        std::auto_ptr<NLLikelihood> nll = model->getNLLikelihood(data);
        double nll_1 = (*nll)(mode);
        ts_producer->produce(data, *model);
        double ts = save_double_products->get_product(ts_name);
        for(int j=0; j<n_poi_scan; ++j){
            double poi_value2 = poi_min + j * poi_step;
            mode.set(poi, poi_value2);
            double nll_2 = (*nll)(mode);
            ordering_table->set_column(*c_truth, poi_value2);
            ordering_table->set_column(*c_ts, ts);
            ordering_table->set_column(*c_ordering, nll_1 - nll_2);
            ordering_table->add_row();
        }
        if(progress_listener) progress_listener->progress(i+1, n_poi_scan);
    }
}

REGISTER_PLUGIN(fclike_ordering)


