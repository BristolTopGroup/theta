#ifndef CUBICLINEAR_HISTOMORPH_HPP
#define CUBICLINEAR_HISTOMORPH_HPP

#include "interface/histogram-function.hpp"
#include "interface/plugin.hpp"


/** \brief A HistogramFunction which interpolates between a "nominal" Histogram and several shifted Histograms
 *
 * Each bin is interpolated independently. Interpolation is done with a cubic spline and extrapolation (beyond the
 * values of the shifted histograms) is done with a linear function.
 *
 * The configuration is very similar to \c interpolating_histo :
 * \code
 *   histogram = {
 *      type = "cubiclinear_histomorph";
 *      parameters = ("delta1", "delta2");
 *      nominal-histogram = { / * fixed-histogram-specification * / };
 *      delta1-plus-histogram = { / * fixed-histogram-specification * / };
 *      delta1-minus-histogram = { / * fixed-histogram-specification * / };
 *      delta2-plus-histogram = { / * fixed-histogram-specification * / };
 *      delta2-minus-histogram = { / * fixed-histogram-specification * / };
 *      //optional settings:
 *      parameter_factors = (0.5, 1.2);
 *      normalize_to_nominal = true; // default is false
 *   };
 * \endcode
 *
 * \c parameters is a list of parameter names used to interpolate between the nominal and the shifted Histograms. For a parameter value of 0.0,
 *    the nominal histograms is reproduced, for a parameter value of +-1, the "-plus-hitsogram" or "-minus-histogram" are reproduced.
 *    Therefore, the parameters usually have a Gaussian prior around 0.0 with width 1.0.
 *
 * For each parameter given in \c parameters, there must be according "-plus-histogram" and "-minus-histogram" settings.
 * \c fixed-histogram-specification is a Histogram Setting block that returns a Histogram which does not depend on any parameters.
 *
 * \c parameter_factors is an optional list of floating point values to multiply the parameter value with before they are used in thye interpolation.
 *  If given, it must have the same length as the \c parameters list as it refers to these parameters, in the same order. If it is not given,
 *  it is equivalent to specifying a list of 1.0.
 *  This setting is useful
 *  if some shifted histograms actually represent other than +-1sigma shifts. For example, setting a parameter_factor to 0.5 while leaving the
 *  prior to Gaussian around 0.0 with width 1.0 efectively means that the shifted shapes are the ones corresponding to +-2sigma.
 *
 * If \c normalize_to_nominal is \c true, the histogram after interpolation will be normalized to the nominal histogram. The default (false) is to
 * interpolate the bin content in each bin and not to change the normalization after the interpolation at all. Setting this parameter to \c true
 * is useful if one wants to treat the normalization uncertainty not as part of the interpolation but rather seperately by the
 * coefficienct-function (which can use the same parameter as used here for histogram interpolation). This provides more flexibility in the
 * handling of the normlization uncertainty.
 *
 * No random fluctuations are done: the method \c getRandomFluctuation is not overriden from \link theta::HistogramFunction HistogramFunction \endlink
 * and will therefore return the same values as the usual, non-random, evaluation operator, \c operator().
 *
 * If this calculation leads to a negative bin entry, it is set to zero to avoid unphysical templates.
 */
class cubiclinear_histomorph : public theta::HistogramFunction {
public:
    
    /** \brief Constructor used by the plugin system to build an instance from settings in a configuration file
     */
    cubiclinear_histomorph(const theta::Configuration & ctx);
        
    /** Returns the interpolated Histogram as documented in the class documentation.
     * throws a NotFoundException if a parameter is missing.
     */
    virtual const theta::Histogram1D & operator()(const theta::ParValues & values) const;
    
    /// Return a Histogram of the same dimenions as the one returned by operator()
    virtual theta::Histogram1D get_histogram_dimensions() const{
       return h0;
    }
    
private:
    
    //cubiclinear_histomorph(const cubiclinear_histomorph & rhs, const theta::PropertyMap & pm);
    /** \brief Build a (constant) Histogram from a Setting block.
    *
    * Will throw an InvalidArgumentException if the Histogram is not constant.
    */
    static theta::Histogram1D getConstantHistogram(const theta::Configuration & ctx, theta::SettingWrapper s);
    
    theta::Histogram1D h0;
    double h0_sum;
    std::vector<theta::Histogram1D> hplus_diff; // hplus_diff[i] + h0 yields hplus
    std::vector<theta::Histogram1D> hminus_diff;
    
    std::vector<double> parameter_factors;
    bool normalize_to_nominal;
    
    //diff and sum are the difference and sum of the hplus_diff and hminus_diff histos
    std::vector<theta::Histogram1D> diff;
    std::vector<theta::Histogram1D> sum;
    //the interpolation parameters used to interpolate between hplus and hminus.
    std::vector<theta::ParId> vid;
    //the Histogram returned by operator(). Defined as mutable to allow operator() to be const.
    mutable theta::Histogram1D h;
    //intermediate histogram for operator()
    mutable theta::Histogram1D diff_total;
};

#endif

