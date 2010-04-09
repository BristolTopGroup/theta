#include "interface/distribution.hpp"

void theta::DistributionUtils::fillModeWidthSupport(theta::ParValues & mode, theta::ParValues & width,
                std::map<theta::ParId, std::pair<double, double> > & support, const theta::Distribution & d){
    ParIds pids = d.getParameters();
    d.mode(mode);
    assert(mode.getAllParIds()==pids);
    for(ParIds::const_iterator p_it=pids.begin(); p_it!=pids.end(); ++p_it){
        width.set(*p_it, d.width(*p_it));
        support[*p_it] = d.support(*p_it);
    }
    assert(width.getAllParIds()==pids);
}