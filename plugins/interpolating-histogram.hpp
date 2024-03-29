#ifndef INTERPOLATING_HISTOGRAM_HPP
#define INTERPOLATING_HISTOGRAM_HPP

#include "interface/histogram-function.hpp"
#include "interface/plugin.hpp"


/** \brief A HistogramFunction which interpolates a "zero" Histogram and several "distorted" Histograms as generic method to treat systematic uncertainties.
 *
 * The configuration is done via:
 * \code
 *   histogram = {
 *      type = "interpolating_histo";
 *      parameters = ("delta1", "delta2");
 *      nominal-histogram = { / * fixed-histogram-specification * / };
 *      delta1-plus-histogram = { / * fixed-histogram-specification * / };
 *      delta1-minus-histogram = { / * fixed-histogram-specification * / };
 *      delta2-plus-histogram = { / * fixed-histogram-specification * / };
 *      delta2-minus-histogram = { / * fixed-histogram-specification * / };
 *   };
 * \endcode
 * Here, <tt>fixed-histogram-specification</tt> is a Histogram Setting block that returns a Histogram
 * which does not depend on any parameters.
 *
 * Bin k of the returned histogram is calculated by multiplying the content of the nominal
 * histogram with either (hplus[i][k]/h0[k])^fabs(p_i) or with (hminus[i][k]/h0[k])^fabs(p_i),
 * where hplus[i][k] is bin k of hplus[i]. Which formula is used depends on the sign of the parameter p_i.
 */
class interpolating_histo : public theta::HistogramFunction {
public:
    
    /** \brief Constructor used by the plugin system to build an instance from settings in a configuration file
     */
    interpolating_histo(const theta::plugin::Configuration & ctx);
        
    /** Returns the interpolated Histogram as documented in the class documentation.
     * throws a NotFoundException if a parameter is missing.
     */
    virtual const theta::Histogram & operator()(const theta::ParValues & values) const;

    /// Return a Histogram of the same dimenions as the one returned by operator()
    virtual theta::Histogram get_histogram_dimensions() const{
        return h0;
    }


    virtual const theta::Histogram & gradient(const theta::ParValues & values, const theta::ParId & pid) const;
private:
    /** \brief Build a (constant) Histogram from a Setting block.
    *
    * Will throw an InvalidArgumentException if the Histogram is not constant.
    */
    static theta::Histogram getConstantHistogram(const theta::plugin::Configuration & ctx, theta::SettingWrapper s);
    
    theta::Histogram h0;
    std::vector<theta::Histogram> hplus;
    std::vector<theta::Histogram> hminus;
    //the interpolation parameters used to interpolate between hplus and hminus.
    std::vector<theta::ParId> vid;
    //the Histogram returned by operator(). Defined as mutable to allow operator() to be const.
    mutable theta::Histogram h;
};

#endif
