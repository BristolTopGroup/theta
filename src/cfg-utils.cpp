#include "interface/cfg-utils.hpp"
#include "interface/histogram-function.hpp"

#include <sstream>

using namespace std;
using namespace libconfig;
using namespace theta;

void SettingUsageRecorder::markAsUsed(const libconfig::Setting & s){
    used_paths.insert(s.getPath());
}

void SettingUsageRecorder::get_unused(std::vector<std::string> & unused, const libconfig::Setting & aggregate_setting) const{
    int n = aggregate_setting.getLength();
    for(int i=0; i<n; ++i){
        std::string path = aggregate_setting[i].getPath();
        if(used_paths.find(path) == used_paths.end()){
            unused.push_back(path);
        }
        if(aggregate_setting[i].isAggregate()){
            get_unused(unused, aggregate_setting[i]);
        }
    }
}

double SettingWrapper::get_double_or_inf() const {
    rec.markAsUsed(setting);
    if(setting.getType()==Setting::TypeFloat) return setting;
    string infstring = setting;
    if(infstring == "inf" || infstring == "+inf") return numeric_limits<double>::infinity();
    if(infstring == "-inf") return -numeric_limits<double>::infinity();
    stringstream error;
    error << "error reading double (or \"inf\") from configuration path " << getPath();
    throw InvalidArgumentException(error.str());
}

const Setting & SettingWrapper::resolve_link(const Setting & setting, const Setting & root, SettingUsageRecorder & rec){
    try{
        std::string next_path;
        //hard-code maximum redirection level of 10:
        for(int i=0; i <= 10; ++i){
            const libconfig::Setting & s = i==0 ? setting : root[next_path];
            if(s.getType() != libconfig::Setting::TypeString) return s;
            std::string link = s;
            if(link.size()==0 || link[0]!='@'){
                return s;
            }
            link.erase(0, 1);
            next_path = link;
            //mark any intermediate link as used:
            rec.markAsUsed(s);
        }
    }
    catch(Exception & ex){
        std::stringstream ss;
        ss << "Exception while trying to resolve link at " << setting.getPath() << ": " << ex.message;
        ex.message = ss.str();
        throw;
    }
    std::stringstream ss;
    ss << "While trying to resolve link at " << setting.getPath() << ": link level is too deep";
    throw ConfigurationException(ss.str());
}

SettingWrapper::SettingWrapper(const libconfig::Setting & s, const libconfig::Setting & root, SettingUsageRecorder & recorder):
           rootsetting(root), rec(recorder), setting(resolve_link(s, rootsetting, rec)){
}
