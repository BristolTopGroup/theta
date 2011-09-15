#include "plugins/textout_database.hpp"
#include "interface/plugin.hpp"
#include "interface/redirect_stdio.hpp"
#include "interface/histogram.hpp"

using namespace std;
using namespace theta;

textout_database::textout_database(const plugin::Configuration & cfg) : save_all_products(true){
    if(cfg.setting.exists("products_data")){
      try{
         string s = cfg.setting["products_data"];
         if(s=="*")save_all_products = true;
         else throw ConfigurationException("products_data setting is a string but not '*'");
      }
      catch(libconfig::SettingTypeException & e){
          save_all_products = false;
          size_t n = cfg.setting["products_data"].size();
          for(size_t i=0; i<n; ++i){
              string column_name = cfg.setting["products_data"][i];
              products_data.insert(column_name);
              if(column_name=="*"){
                 save_all_products = true;
                 products_data.clear();
                 break;
              }
          }
          //if anything is written at all, also write runid and eventid:
          if(products_data.size()){
             products_data.insert("runid");
             products_data.insert("eventid");
          }
      }
    }
}

textout_database::~textout_database() {
}


std::auto_ptr<Table> textout_database::create_table(const string & table_name){
    check_name(table_name);
    textout_table * result = new textout_table(table_name, boost::dynamic_pointer_cast<textout_database>(shared_from_this()));
    if(table_name == "products"){
        result->save_all_columns = save_all_products;
        result->save_columns = products_data;
    }
    return std::auto_ptr<Table>(result);
}


textout_database::textout_table::textout_table(const string & name_, const boost::shared_ptr<textout_database> & db_) : Table(db_), next_colid(0), irow(0),
    name(name_), db(db_), save_all_columns(true) {
}

Column textout_database::textout_table::add_column(const std::string & name, const data_type & type){
    if(!save_all_columns && save_columns.find(name) == save_columns.end()) return Column(-1);
    Column result(next_colid++);
    column_infos[result] = column_info(name, type);
    return result;
}

ostream & operator<<(ostream & out, const theta::Histogram & h){
    out << "histogram(nbins=" << h.get_nbins() << ",xmin=" << h.get_xmin() << ",xmax=" << h.get_xmax() << ",data=[";
    for(size_t i=0; i<=h.get_nbins(); ++i){
        out << h.get(i) << ",";
    }
    return out << h.get(h.get_nbins()+1) << "])";
}

void textout_database::textout_table::add_row(const Row & row){
    theta::cout << endl << "Table '" << name << "', row " << ++irow << ":" << endl;
    for(int i=0; i<next_colid; ++i){
        Column c(i);
        theta::cout << column_infos[c].name << "=";
        data_type type = column_infos[c].type;
        switch(type){
            case typeInt:
                theta::cout << row.get_column_int(c);
                break;
            case typeDouble:
                theta::cout << row.get_column_double(c);
                break;
            case typeString:
                theta::cout << row.get_column_string(c);
                break;
            case typeHisto:
                theta::cout << row.get_column_histogram(c);
                break;
        }
        theta::cout << endl;
        //<< current_col_values[i] << endl;
    }
}

REGISTER_PLUGIN(textout_database)
