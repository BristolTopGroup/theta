#include "interface/cfg-utils.hpp"
#include "interface/histogram-function.hpp"

#include <sstream>

using namespace std;
using namespace libconfig;

namespace theta{ namespace utils{

void remove_empty_groups(Setting & s){
    for(int i=0; i<s.getLength(); i++){
        remove_empty_groups(s[i]);
        if(s[i].isGroup() && s[i].getLength()==0){
            s.remove(s[i].getName());
            --i;
        }
    }
}

vector<string> get_paths(Setting & s){
    vector<string> result;
    for(int i=0; i<s.getLength(); i++){
        if(s[i].getLength()==0) result.push_back(s[i].getPath());
        else{
            vector<string> tmp = get_paths(s[i]);
            result.insert(result.end(), tmp.begin(), tmp.end());
        }
    }
    return result;
}

double get_double_or_inf(Setting & s){
    if(s.getType()==Setting::TypeFloat) return s;
    string infstring = s;
    if(infstring == "inf" || infstring == "+inf") return numeric_limits<double>::infinity();
    if(infstring == "-inf") return -numeric_limits<double>::infinity();

    stringstream error;
    error << "error reading double (or \"inf\") from configuration path " << s.getPath();
    throw InvalidArgumentException(error.str());
}


}}
