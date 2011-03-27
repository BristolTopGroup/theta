#include "interface/main.hpp"
#include "interface/database.hpp"
#include "interface/plugin.hpp"
#include "interface/model.hpp"

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <vector>
#include <set>

#include <boost/bimap/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>

using namespace std;
using namespace boost::bimaps;
using namespace libconfig;
using namespace theta;
using namespace theta::plugin;


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


//t_input_data is a bimap between
// (ts values) <-> (ordering values)
// both can contain the same value multiple time, i.e., both are non-unique.
//
//As even the pair (ts value, ordering value) might not be unique, use unconstrained_set_of_relation 
// as relation view.
//
// The ordered view on (ts value) is useful for constructing the ts interval.
// The ordered view on (ordering values) is useful to find the interval seed in the interval construction.
typedef bimap<multiset_of<double>, multiset_of<double>, unconstrained_set_of_relation > t_input_data;



/** \brief Neyman belt construction
 *
 * The Neyman construction for confidence intervals works by defining a <em>confidence belt</em>
 * on the (truth, test statistic) plane, given the result of toy experiments as input. Here, "truth"
 * refers to the true value of the parameter of interest, "test statistic" (ts) is a real-valued random
 * variable used to discriminate (on a per-toy basis) between different values for "truth". Typical choices
 * for the test statistic are the result of a maximum likelihood estimate for the parameter of interest (via the mle plugin)
 * or a likelihood ratio (via the deltanll_hypotest plugin). However, it is also possible to use some
 * completely different things such as the quantiles of the Bayesian posterior. The latter has an interesting use case
 * as the Neyman construction using Bayesian quatiles as test statistic can be interpreted as coverage test
 * for the Bayesian method, at least for one-sided intervals.
 *
 * The Neyman construction uses the result of a previous theta run as input: the input database
 * is expected to contain, in each row of the "products" table, the values for this toy for "truth"
 * and "test statistic".
 *
 * Important: the values of the "truth" column in the input must be discrete. It is recommended to
 * use \link equidistant_deltas\endlink for the parameter of interest to make the input.
 *
 * The constructed belt will have (at least) the desired coverage on the ensemble used as input.
 *
 * \code
 * main = {
 *   type = "neyman_belt";
 *   cls = [0.68, 0.95];
 *   ordering_rule = "fclike";
 *   // fclike_options is only required if ordering_rule is "fclike".
 *   fclike_options = {
 *      model = "@some_model";
 *      ts_producer = "@mle";    // use the same producer as for the ts_column in toy_database below
 *      truth = "beta_signal";   // parameter name of the "truth"
 *   };
 *   force_increasing_belt = true; // optional; default is false
 *   // input:
 *   toy_database = {
 *      type = "sqlite_database_in";
 *      filename = "toys.db";
 *   };
 *   truth_column = "source__beta_signal";
 *   ts_column = "mle__beta_signal";
 *   // output:
 *   output_database = {
 *      type = "sqlite_database";
 *      filename = "belt.db";
 *   };
 * };
 * \endcode
 *
 * \c type must always be "neyman_belt" to select this plugin
 *
 * \c cls are the confidence levels for which to construct the belts
 *
 * \c ordering_rule is the ordering rule to use for the belt construction. It controls which
 *   toys are included in the belt first. Valid values are 'central', 'central_shortest', 'lower', 'upper' and 'fclike'. If choosing
 *   'fclike', the additional setting group 'fclike_options' has to be given. It has to specify the model, the test statistic producer and
 *   parameter name for "truth".
 *
 * \c force_increasing_belt controls whether to the belt borders in "ts" as function of "truth" are forced to be increasing.
 *
 * \c toy_database is the input database in which the information about the toys to use for the construction is stored.
 *  \c truth_column and \c ts_column are the column names in this database for the value for "truth" and "ts", respectively.
 *
 * \c output_database configures where the result is written.
 *
 * For each value of "truth" in toy_database, the interval in "ts" is constructed which belongs to the belt. For the construction, an algorithm
 * is used which ensures that this set of points is always an interval. For ordering_rule 'lower' and 'upper', the resulting interval extends
 * to +-infinity in one side and +-infinity will be written to the result table for that interval end, even if this is not a
 * valid value for "ts".
 *
 * The result is written in a table named "belt" with (3*n + 1) columns if n is the number of confidence levels configured. The column
 * names are "truth" and for each confidence level "ts_lower<cl>", "ts_upper<cl>" and "coverage<cl>" where "<cl>" is the confidence level
 * as string using 5 digits, placing the decimal point after the first one. For example, a confidence level 0.68 will create the columns
 * "ts_lower06800", "ts_upper06800", and "coverage06800". For each row, the ts_lower and ts_upper columns specify the "ts" interval included in the belt.
 * The coverage column contains the coverage of this interval. 
 */
class neyman_belt: public theta::Main{
public:
    neyman_belt(const plugin::Configuration & cfg);
    virtual void run();
    virtual ~neyman_belt(){}
private:
    vector<double> cls;
    string ordering_rule;
    bool force_increasing_belt;
    
    boost::shared_ptr<DatabaseInput> toy_database;
    string truth_column, ts_column;
    
    boost::shared_ptr<Database> output_database;
    
    // fclike options:
    boost::shared_ptr<Model> model;
    std::auto_ptr<ParId> poi;
    boost::shared_ptr<Producer> ts_producer;
    boost::shared_ptr<SaveDoubleProducts> save_double_products;
    std::string ts_name;
    
    void add_ordering_fclike(map<double, t_input_data> & inp);
};

// There are several potential problems in belt construction:
// P1. by subsequently adding ts points, sorted by ordering value, the resulting ts set will not be, in general, an interval in ts
// P2. the actually taken ts value might be discrete in theory, with some numerical differences in practice
//    (for example, the maximum likelihood estimate in a counting experiment with known background b is max(0, n_obs - b) which
//    is discrete)
//
// Therefore, the algorithm to construct the intervals is a bit involved.
// A1. start the interval in ts by taking the ts value which has lowest (smoothed) ordering value
// A2. subsequently add ts points either on the left or right side of the current interval, preferring toys with low ordering value.
//     Once a point is added, also add points with the same / very similar value of ts. This addresses P2. and P3.
// A3. to address P2., the actual coverage reported in the output is calculated for an interval which is enlarged by 1% (however,
//     the cited interval is the one originally found). This ensures that the reported coverage for the interval is more robust
//     against numerical roundoff issues
//
// The steps A1 to A3 are taken for each truth value, in increasing order of the truth value.
// During this construction, the lower and upper ts interval ends are both forced to be non-decreasing
//  as function of the truth value (unless a decrease is necessary to fulfill coverage).



// construct a belt interval; input_data is for a single truth value.
// the returned pair is an interval in the test statistic.
// ts_interval0_min and ts_interval1_min are the minimum values
//   for the lower and upper interval ends in the ts value. This is
//   set to the result of the previous interval to ensure that the resulting
//   belt yields actual confidence intervals (not just regions). Can be set
//   to -infinity to not impose any restrictions.
//   Note that these restrictions are not kept if coverage would be lost.
struct t_interval_coverage{
    pair<double, double> interval;
    double coverage;
};
t_interval_coverage construct_interval(const t_input_data & input_data, double cl, double ts_interval0_min, double ts_interval1_min){
    const size_t n_target = static_cast<size_t>(cl * input_data.left.size());
    //take the ts value corresponding to the lowest ordering value which respects the restrictions as starting point for the interval:
    pair<double, double> result;
    t_input_data::right_map::const_iterator it_seed = input_data.right.begin();
    while(it_seed->second < ts_interval0_min && it_seed != input_data.right.end()){
        ++it_seed;
    }
    if(it_seed==input_data.right.end()){
       //no seed found which fulfils the requirements.
       it_seed = input_data.right.begin();
       result.first = result.second = it_seed->second;
    }
    else{
       result.first = result.second = it_seed->second;
       result.first = max(result.first, ts_interval0_min);
       result.second = max(result.second, ts_interval1_min);
    }
    assert(result.second >= result.first);

    // A.3. add ajacent values directly left of or right of the result interval, preferring the value according to the ordering.
    // Repeat until coverage is reached.
    // Interval contains the values in [it_min, it_max), i.e., excludes where it_max points to; in particular,
    //   it_max might point to the input_data.left.end().
    t_input_data::left_map::const_iterator it_min = input_data.left.lower_bound(result.first);
    t_input_data::left_map::const_iterator it_max = input_data.left.upper_bound(result.second);
    size_t n = distance(it_min, it_max);
    assert(n > 0);
    //cout << "using interval seed [" << result.first << " " << result.second << "] which contains " << n << " points; n_target is " << n_target << endl;
    const double nan = numeric_limits<double>::quiet_NaN();
    while(n < n_target){
        double order_value_left = nan, order_value_right = nan;
        double ts_value_left = nan, ts_value_right = nan;
        if(it_min != input_data.left.begin()){
            t_input_data::left_map::const_iterator it_left = it_min;
            --it_left;
            order_value_left = it_left->second;
            ts_value_left = it_left->first;
        }
        if(it_max != input_data.left.end()){
            order_value_right = it_max->second;
            ts_value_right = it_max->first;
        }
        bool left = false, right = false;
        if(isnan(order_value_right)){
           if(isnan(order_value_left)){
               throw string("no valid range found!");
           }
           else{
              --it_min; ++n; result.first = ts_value_left; left = true;
           }
        }
        else{
           if(isnan(order_value_left)){
              ++it_max; ++n; result.second = ts_value_right; right = true;
           }
           else{
               if(order_value_left < order_value_right && ts_value_left >= ts_interval0_min){--it_min; ++n; result.first = ts_value_left; left = true;}
               else { ++it_max; ++n; result.second = ts_value_right; right = true; }
           }
        }
        //A.3.a. Add points with same / very similar ts value of the one just added
        double ilen = result.second - result.first;
        if(left){
            while(it_min != input_data.left.begin()){
                t_input_data::left_map::const_iterator it_left = it_min;
                --it_left;
                if(fabs(it_left->first - ts_value_left) <= ilen * 0.001){
                    --it_min; ++n;
                    result.first = it_min->first;
                }
                else break;
            }
        }
        if(right){
            while(it_max != input_data.left.end() && fabs(it_max->first - ts_value_right) <= ilen * 0.001){
                result.second = it_max->first;
                ++it_max; ++n;
            }
        }
    }
    t_interval_coverage res;
    res.interval = result;
    res.coverage = n * 1.0 / input_data.left.size();
    return res;
}



namespace{

// returns map (true value) -> input data; all ordering values are set to 0.0
map<double, t_input_data> get_input(DatabaseInput & db, const string & truth_column, const string & ts_column){
   map<double, t_input_data> result;
   vector<string> colnames;
   colnames.push_back(truth_column);
   colnames.push_back(ts_column);
   std::auto_ptr<DatabaseInput::ResultIterator> it = db.query("products", colnames);
   double ordering = 0;
   size_t i = 0;
   while(it->has_data()){
       ++i;
       double truth = it->get_double(0);
       double ts = it->get_double(1);
       result[truth].left.insert(t_input_data::left_map::value_type(ts, ordering));
       ++(*it);
   }
   cout << "get_input: read " << i << " rows" << endl;
   return result;
}

void add_ordering_lu(map<double, t_input_data> & inp, const std::string & ordering_rule){
     double o_diff = 1;
     if(ordering_rule == "upper") o_diff = -1;
     for(map<double, t_input_data>::iterator it=inp.begin(); it!=inp.end(); ++it){
         t_input_data & input_data = it->second;
         t_input_data new_input_data;
         double o = 0;
         t_input_data::left_map::const_iterator it_begin = input_data.left.begin();
         t_input_data::left_map::const_iterator it_end = input_data.left.end();
         for(t_input_data::left_map::const_iterator it=it_begin; it!=it_end; ++it){
             new_input_data.left.insert(t_input_data::left_map::value_type(it->first, o)); 
             o += o_diff;
         }
         swap(input_data, new_input_data);
     }
}

void add_ordering_central(map<double, t_input_data> & inp){
    for(map<double, t_input_data>::iterator it=inp.begin(); it!=inp.end(); ++it){
        t_input_data & input_data = it->second;
        t_input_data new_input_data;
        t_input_data::left_map::const_iterator it_begin = input_data.left.begin();
        t_input_data::left_map::const_iterator it_end = input_data.left.end();
        t_input_data::left_map::const_iterator it_left = it_begin;
        advance(it_left, input_data.left.size() / 2);
        t_input_data::left_map::const_iterator it_right = it_left;
        double o = 0;
        while(it_left!=it_begin || it_right!=it_end){
            if(it_left!=it_begin){
                --it_left;
                new_input_data.left.insert(t_input_data::left_map::value_type(it_left->first, o++));
            }
            if(it_right!=it_end){
                new_input_data.left.insert(t_input_data::left_map::value_type(it_right->first, o++));
                ++it_right;
            }
        }
        swap(input_data, new_input_data);
    }
}

void add_ordering_central_shortest(map<double, t_input_data> & inp){
    for(map<double, t_input_data>::iterator it=inp.begin(); it!=inp.end(); ++it){
        t_input_data & input_data = it->second;
        t_input_data new_input_data;
        t_input_data::left_map::const_iterator it_begin = input_data.left.begin();
        t_input_data::left_map::const_iterator it_end = input_data.left.end();
        t_input_data::left_map::const_iterator it_left = it_begin;
        advance(it_left, input_data.left.size() / 2);
        t_input_data::left_map::const_iterator it_right = it_left;
        //it_right points one after the last inserted element (i.e., to the next to be inserted).
        // it_left points to the last inserted element.
        double o = 0;
        new_input_data.left.insert(t_input_data::left_map::value_type(it_right->first, o++));
        double ts0 = it_right->first;
        ++it_right;
        while(it_left!=it_begin || it_right!=it_end){
            if(it_left!=it_begin && it_right!=it_end){
                t_input_data::left_map::const_iterator it_left_next = it_left;
                --it_left_next;
                double next_ts_left = it_left_next->first;
                double next_ts_right = it_right->first;
                if(fabs(next_ts_left - ts0) < fabs(next_ts_right - ts0)){
                    --it_left;
                    new_input_data.left.insert(t_input_data::left_map::value_type(it_left->first, o++));
                }
                else{
                     new_input_data.left.insert(t_input_data::left_map::value_type(it_right->first, o++));
                    ++it_right;
                }
            }
            else if(it_left!=it_begin){
                --it_left;
                new_input_data.left.insert(t_input_data::left_map::value_type(it_left->first, o++));
            }
            else if(it_right!=it_end){
                new_input_data.left.insert(t_input_data::left_map::value_type(it_right->first, o++));
                ++it_right;
            }
        }
        swap(input_data, new_input_data);
    }
}

}


void neyman_belt::add_ordering_fclike(map<double, t_input_data> & inp){
    ParValues mode;
    std::map<theta::ParId, std::pair<double, double> > support;
    DistributionUtils::fillModeSupport(mode, support, model->get_parameter_distribution());
    //get the set of true values of the poi:
    set<double> truth_values;
    for(map<double, t_input_data>::const_iterator it=inp.begin(); it!=inp.end(); ++it){
        truth_values.insert(it->first);
    }
    map<double, map<double, double> > truth_to__ts_to_ordering;
    Data data;
    // for ts calculation, go to higher turth values:
    double mi = *(min_element(truth_values.begin(), truth_values.end()));
    double ma = *(max_element(truth_values.begin(), truth_values.end()));
    const size_t N = 200;
    double stepsize = 2 * (ma - mi) / N;
    for(size_t i=0; i<N; ++i){
        if(progress_listener) progress_listener->progress(i, truth_values.size() + N);
        //use "it" to generate asimov data, calculate the ts and nll_best:
        double poi_value1 = mi + i * stepsize;
        mode.set(*poi, poi_value1);
        model->get_prediction(data, mode);
        std::auto_ptr<NLLikelihood> nll = model->getNLLikelihood(data);
        double nll_best = (*nll)(mode);
        ts_producer->produce(data, *model);
        double ts = save_double_products->get_product(ts_name);
        //it2 is the "truth":
        for(set<double>::const_iterator it2=truth_values.begin(); it2!=truth_values.end(); ++it2){
            double truth = *it2;
            mode.set(*poi, truth);
            double nll_2 = (*nll)(mode);
            truth_to__ts_to_ordering[truth][ts] = nll_2 - nll_best;
        }
    }
    //now go through all truth values again and use truth_to__ts_to_ordering
    // to interpolate / extrapolate ...
    int c = 0;
    for(map<double, t_input_data>::iterator it=inp.begin(); it!=inp.end(); ++it, ++c){
        if(progress_listener) progress_listener->progress(N + c, truth_values.size() + N);
        //t_input data is    ts (left) -> ordering (right)
        t_input_data & input_data = it->second;
        t_input_data new_input_data;
        const double truth = it->first;
        const map<double, double> & ts_to_ordering = truth_to__ts_to_ordering[truth];
        assert(ts_to_ordering.size() >= 2);
        for(t_input_data::left_map::const_iterator i=input_data.left.begin(); i!=input_data.left.end(); ++i){
            const double ts = i->first;
            //find ordering value from ts_to_ordering by linear interpolation:
            map<double, double>::const_iterator it1 = ts_to_ordering.upper_bound(ts);
            map<double, double>::const_iterator it2 = it1;
            //make it1, it2 point to valid elements not at the beginning:
            if(it2==ts_to_ordering.end()){
               --it2;
               --it1;
            }
            if(it1==ts_to_ordering.begin()){
                ++it1;
                ++it2;
            }
            --it1;
            const double ts_low = it1->first;
            const double ts_high = it2->first;
            const double ordering_low = it1->second;
            const double ordering_high = it2->second;
            double ordering = ordering_low + (ts - ts_low) * (ordering_high - ordering_low) / (ts_high - ts_low);
            new_input_data.left.insert(t_input_data::left_map::value_type(ts, ordering));
        }
        swap(it->second, new_input_data);
    }
    if(progress_listener) progress_listener->progress(N + c, N + c);
}


neyman_belt::neyman_belt(const plugin::Configuration & cfg): force_increasing_belt(false){
    size_t n = cfg.setting["cls"].size();
    if(n==0) throw ConfigurationException("'cls' must be a non-empty list");
    for(size_t i=0; i<n; ++i){
        cls.push_back(cfg.setting["cls"][i]);
    }
    ordering_rule = static_cast<string>(cfg.setting["ordering_rule"]);
    if(ordering_rule!="lower" && ordering_rule != "upper" && ordering_rule != "central"
         && ordering_rule != "central_shortest" && ordering_rule != "fclike"){
          throw ConfigurationException("unknown ordering_rule '" + ordering_rule + "'");
    }
    if(cfg.setting.exists("force_increasing_belt")){
        force_increasing_belt = cfg.setting["force_increasing_belt"];
    }
    
    toy_database = plugin::PluginManager<DatabaseInput>::instance().build(Configuration(cfg, cfg.setting["toy_database"]));
    truth_column = static_cast<string>(cfg.setting["truth_column"]);
    ts_column = static_cast<string>(cfg.setting["ts_column"]);
    
    output_database = plugin::PluginManager<Database>::instance().build(Configuration(cfg, cfg.setting["output_database"]));
    if(ordering_rule == "fclike"){
        save_double_products.reset(new SaveDoubleProducts());
        cfg.pm->set<ProductsSink>("default", save_double_products);
        model = plugin::PluginManager<Model>::instance().build(Configuration(cfg, cfg.setting["fclike_options"]["model"]));
        poi.reset(new ParId(cfg.vm->getParId(cfg.setting["fclike_options"]["truth"])));
        ts_producer = plugin::PluginManager<Producer>::instance().build(Configuration(cfg, cfg.setting["fclike_options"]["ts_producer"]));
        //ts_name is the "module only" part of ts_column:
        size_t p = ts_column.find("__");
        if(p==string::npos){
            throw ConfigurationException("ts_column should contain '__'");
        }
        ts_name = ts_column.substr(p+2);

    }
}

void neyman_belt::run(){
    cout << "reading input" << endl;
    map<double, t_input_data> inp = get_input(*toy_database, truth_column, ts_column);
    cout << "calculating ordering" << endl;
    if(ordering_rule=="lower" || ordering_rule=="upper"){
        add_ordering_lu(inp, ordering_rule);
    }
    else if(ordering_rule=="central"){
        add_ordering_central(inp);
    }
    else if(ordering_rule=="central_shortest"){
        add_ordering_central_shortest(inp);
    }
    else{
       assert(ordering_rule=="fclike" && model.get()!=0 && poi.get()!=0 && ts_producer!=0);
       add_ordering_fclike(inp);
    }
    cout << "finding intervals" << endl;
    //find intervals:
    std::auto_ptr<Table> belt_table = output_database->create_table("belt");
    std::auto_ptr<Column> c_truth = belt_table->add_column("truth", typeDouble);
    boost::ptr_vector<Column> c_lower, c_upper, c_coverage;
    for(size_t i=0; i<cls.size(); ++i){
        stringstream suffix;
        suffix << setw(5) << setfill('0') << static_cast<int>(cls[i] * 10000 + 0.5);
        c_lower.push_back(belt_table->add_column("ts_lower" + suffix.str(), typeDouble));
        c_upper.push_back(belt_table->add_column("ts_upper" + suffix.str(), typeDouble));
        c_coverage.push_back(belt_table->add_column("coverage_upper" + suffix.str(), typeDouble));
    }
    
    size_t n_input = inp.size();
    size_t n_cls = cls.size();
    const double inf = numeric_limits<double>::infinity();
    for(size_t i=0; i<cls.size(); ++i){
        pair<double, double> previous_interval;
        previous_interval.first = previous_interval.second = -numeric_limits<double>::infinity();
        const double cl = cls[i];
        size_t i_input=0;
        for(map<double, t_input_data>::iterator it=inp.begin(); it!=inp.end(); ++it, ++i_input){
            if(progress_listener)progress_listener->progress(i * n_input + i_input, n_cls * n_input);
            //relax the contraints of increasing intervals by one permille to circumvent rounding issues
            // arising from discrete ts values:
            double l = 0.0;
            if(!isinf(previous_interval.first) && !isinf(previous_interval.second)){
                l = 0.001 * (previous_interval.second - previous_interval.first);
            }
            t_interval_coverage interval = construct_interval(it->second, cl, previous_interval.first - l, previous_interval.second - l);
            if(ordering_rule=="lower"){
                interval.interval.second = inf;
            }
            else if(ordering_rule=="upper"){
                interval.interval.first = -inf;
            }
            belt_table->set_column(*c_truth, it->first);
            belt_table->set_column(c_lower[i], interval.interval.first);
            belt_table->set_column(c_upper[i], interval.interval.second);
            belt_table->set_column(c_coverage[i], interval.coverage);
            belt_table->add_row();
            if(force_increasing_belt){
               previous_interval.first = max(interval.interval.first, previous_interval.first);
               previous_interval.second = max(interval.interval.second, previous_interval.second);
            }
           //if force_increasing_belt is false, previous_interval is never updated and stays at -infinity, i.e., no restriction.
        }
    }
    if(progress_listener)progress_listener->progress(n_cls * n_input, n_cls * n_input);
}

REGISTER_PLUGIN(neyman_belt)

