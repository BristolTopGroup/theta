#include "interface/phys.hpp"

using namespace theta;

/* DATA */
ObsIds Data::getObservables() const{
    ObsIds result;
    std::map<ObsId, Histogram>::const_iterator it = data.begin();
    for(;it!=data.end(); it++){
        result.insert(it->first);
    }
    return result;
}

