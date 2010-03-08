#ifndef INTERPOLATING_HISTOGRAM_HPP
#define INTERPOLATING_HISTOGRAM_HPP

#include "interface/histogram-function.hpp"
#include "interface/plugin.hpp"


/** \brief A HistogramFunction which interpolates a "zero" Histogram and several "distorted" Histograms as generic method to treat systematic uncertainties.
 *
 * The configuration block is something like (mass is some observable previously defined.
 * s, delta1, delta2 are parameters):
 * <pre>
 * mass = {
 *   coefficients = ("s");
 *   histogram = {
 *      type = "interpolating_histo";
 *      parameters = ("delta1", "delta2");
 *      nominal-histogram = { / * fixed-histogram-specification * / };
 *      delta1-plus-histogram = { / * fixed-histogram-specification * / };
 *      delta1-minus-histogram = { / * fixed-histogram-specification * / };
 *      delta2-plus-histogram = { / * fixed-histogram-specification * / };
 *      delta2-minus-histogram = { / * fixed-histogram-specification * / };
 *   };
 * };
 * </pre>
 * Here, <tt>fixed-histogram-specification</tt> is a Histogram Setting block that returns a Histogram
 * which does not depend on any parameters. That is, any Histogram of type="fixed_*" (if the naming convention
 * is obeyed).
 *
 * A histogram \c h0 is interpolated by parameters p_i specified in varids such that
 *  -# if all p_i = 0, the original Histogram h0 is returned
 *  -# for p_j = 1 for fixed j and p_i=0 for all i!=j, the Histogram hplus[j] is returned
 *  -# same with other sign: for p_j=-1 and all other p_i=0, hminus[j] is returned.
 *  -# for all other values, the histogram contents are interpolated binwise: the bin value of bin k is calculated
 *    with (hplus[i][k]/h0[k])^fabs(p_i) * (hminus[j][k]/h0[k])^fabs(p_j), where hplus[i][k] is bin k of hplus[i]. It was assumed
 *    that p_i > 0 and p_j &lt; 0. You can verify that 1., 2. and 3. are fulfilled with this formula.
 *
 * It is not allowed to use the same parameter as interpolating parameter twice.
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

    virtual theta::ParIds getParameters() const;

    virtual bool dependsOn(const theta::ParId & pid) const {
        return std::find(vid.begin(), vid.end(), pid) != vid.end();
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
