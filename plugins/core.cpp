#include "core.hpp"
#include "plugins/interpolating-histogram.hpp"
#include "plugins/run_plain.hpp"

#include <iostream>

using namespace theta;
using namespace theta::plugin;
using namespace libconfig;
using namespace std;

std::auto_ptr<HistogramFunction> FixedPolyHistoFactory::build(ConfigurationContext & ctx) const {
    const Setting & s = ctx.setting;
    ObsId obs_id = ctx.vm->getObsId(s["observable"]);
    ctx.rec.markAsUsed(s["observable"]);
    int order = s["coefficients"].getLength() - 1;
    if (order == -1) {
        stringstream ss;
        ss << "Empty definition of coefficients for polynomial on line " << s["coefficients"].getSourceLine();
        throw ConfigurationException(ss.str());
    }
    size_t nbins = ctx.vm->get_nbins(obs_id);
    pair<double, double> range = ctx.vm->get_range(obs_id);
    Histogram h(nbins, range.first, range.second);
    vector<double> coeffs(order + 1);
    for (int i = 0; i <= order; i++) {
        coeffs[i] = s["coefficients"][i];
    }
    for (size_t i = 1; i <= nbins; i++) {
        double x = h.get_bincenter(i);
        double value = coeffs[order];
        for (int k = 0; k < order; k++) {
            value *= x;
            value += coeffs[order - k - 1];
        }
        h.set(i, value);
    }
    double norm_to = s["normalize_to"];
    double norm;
    if ((norm = h.get_sum_of_weights()) == 0.0) {
        throw ConfigurationException("Histogram specification is zero (can't normalize)");
    }
    h *= norm_to / norm;
    ctx.rec.markAsUsed(s["coefficients"]);
    ctx.rec.markAsUsed(s["normalize_to"]);
    return std::auto_ptr<HistogramFunction > (new ConstantHistogramFunction(h));
}

std::auto_ptr<HistogramFunction> FixedGaussHistoFactory::build(ConfigurationContext & ctx) const {
    const Setting & s = ctx.setting;
    double width = s["width"];
    double mean = s["mean"];
    ObsId obs_id = ctx.vm->getObsId(s["observable"]);
    size_t nbins = ctx.vm->get_nbins(obs_id);
    pair<double, double> range = ctx.vm->get_range(obs_id);
    Histogram h(nbins, range.first, range.second);
    //fill the histogram:
    for (size_t i = 1; i <= nbins; i++) {
        double d = (h.get_bincenter(i) - mean) / width;
        h.set(i, exp(-0.5 * d * d));
    }
    double norm_to = s["normalize_to"];
    double norm;
    if ((norm = h.get_sum_of_weights()) == 0.0) {
        throw ConfigurationException("Histogram specification is zero (can't normalize)");
    }
    h *= norm_to / norm;
    ctx.rec.markAsUsed(s["width"]);
    ctx.rec.markAsUsed(s["mean"]);
    ctx.rec.markAsUsed(s["normalize_to"]);
    return std::auto_ptr<HistogramFunction > (new ConstantHistogramFunction(h));
}

REGISTER_FACTORY(FixedPolyHistoFactory)
REGISTER_FACTORY(FixedGaussHistoFactory)

/*void get_histogram_function_factories(boost::ptr_vector<HistogramFunctionFactory> & factories) {
    factories.push_back(new FixedPolyHistoFactory());
    factories.push_back(new FixedGaussHistoFactory());
    factories.push_back(new InterpolatingHistoFactory());
}

void get_run_factories(boost::ptr_vector<RunFactory> & factories){
    factories.push_back(new PlainRunFactory());
}*/

