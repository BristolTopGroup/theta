#include "interface/phys.hpp"

using namespace theta;

REGISTER_PLUGIN_BASETYPE(Function);
REGISTER_PLUGIN_BASETYPE(DataSource);

/* DATA */
ObsIds Data::getObservables() const{
    ObsIds result;
    std::map<ObsId, Histogram>::const_iterator it = data.begin();
    for(;it!=data.end(); it++){
        result.insert(it->first);
    }
    return result;
}

