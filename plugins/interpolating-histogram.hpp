#ifndef INTERPOLATING_HISTOGRAM_HPP
#define INTERPOLATING_HISTOGRAM_HPP

#include "interface/histogram-function.hpp"
#include "interface/plugin.hpp"


/** A HistogramFunction which interpolates a "zero" Histogram and several "distorted" Histograms.
 *
 * A histogram \c h0 is interpolated by parameters p_i specified in varids such that
 *  -# if all p_i = 0, the original Histogram h0 is returned
 *  -# for p_j = 1 for fixed j and p_i=0 for all i!=j, the Histogram hplus[j] is returned
 *  -# same with other sign: for p_j=-1 and all other p_i=0, hminus[j] is returned.
 *  -# for all other values, the histogram contents are interpolated binwise: the bin value of bin k is calculated
 *    with (hplus[i][k]/h0[k])^fabs(p_i) * (hminus[j][k]/h0[k])^fabs(p_j), where hplus[i][k] is bin k of hplus[i]. It was assumed
 *    that p_i > 0 and p_j &lt; 0. You can verify that 1., 2. and 3. are fulfilled with this formula.
 *
 * It is not allowed to use the same parameter as interpolating parameter twice (i.e., each \c ParId in \c varids must be
 *  unique in that collection).
 *
 * If the number of varids, hplus or hminus Histograms differ, an InvalidArgumentException is thrown.
 * An InvalidArgumentException is also thrown if the Histograms (h0, hplus, hminus) are not compatible.
 *
 * \sa Histogram::checkCompatibility VarId
 */
class InterpolatingHistogramFunction : public theta::HistogramFunction {
public:
    InterpolatingHistogramFunction(const theta::Histogram & h0_, const std::vector<theta::ParId> & varids,
            const std::vector<theta::Histogram> & hplus_, const std::vector<theta::Histogram> & hminus_);
    
    /** Returns the interpolated Histogram as documented in the class documentation.
     * throws a NotFoundException if a parameter is missing.
     */
    virtual const theta::Histogram & operator()(const theta::ParValues & values) const;

    virtual theta::ParIds getParameters() const;

    virtual bool dependsOn(const theta::ParId & pid) const {
        return std::find(vid.begin(), vid.end(), pid) != vid.end();
    }

    virtual const theta::Histogram & gradient(const theta::ParValues & values, const theta::ParId & pid) const;

    virtual ~InterpolatingHistogramFunction() {
    }
private:
    theta::Histogram h0;
    std::vector<theta::Histogram> hplus;
    std::vector<theta::Histogram> hminus;
    //the interpolation parameters used to interpolate between
    // hplus and hminus.
    std::vector<theta::ParId> vid;
    //the Histogram returned by operator(). Defined as mutable to allow
    // operator() to be const.
    mutable theta::Histogram h;
    //virtual HistogramFunction* clone() const;
};

/** \brief Plugin Factory class to produce InterpolatinHistogramFunction of type="interpolating".
 *
 * For a description of the InterpolatingHistogramFunction semantics, see there. In short: it interpolates
 * (previously known, fixed) Histograms, mainly intended for studying systematic uncertainties on the templates.
 *
 * The configuration block is something like (mass is some observable previously defined.
 * s, delta1, delta2 are parameters):
 * <verbatim>
 * mass = {
 *   coefficients = ("s");
 *   histogram = {
 *      type = "interpolating";
 *      parameters = ("delta1", "delta2");
 *      nominal-histogram = { /<b></b>* fixed-histogram-specification *<b></b>/ };
 *      delta1-plus-histogram = { /<b></b>* fixed-histogram-specification *<b></b>/ };
 *      delta1-minus-histogram = { /<b></b>* fixed-histogram-specification *<b></b>/ };
 *      delta2-plus-histogram = { /<b></b>* fixed-histogram-specification *<b></b>/ };
 *      delta2-minus-histogram = { /<b></b>* fixed-histogram-specification *<b></b>/ };
 *   };
 * };
 * </verbatim>
 * Here, <tt>fixed-histogram-specification</tt> is a Histogram Setting block that returns a Histogram
 * which does not depend on any parameters. That is, any Histogram of type="fixed-*" (if the naming convention
 * is obeyed).
 */
class InterpolatingHistoFactory: public theta::plugin::HistogramFunctionFactory{
public:
   virtual std::auto_ptr<theta::HistogramFunction> build(theta::plugin::ConfigurationContext & ctx) const;
   virtual std::string getTypeName() const{
      return "interpolate";
   }
private:
    /** \brief Build a (constant) Histogram from a Setting block.
     */
    static theta::Histogram getConstantHistogram(theta::plugin::ConfigurationContext & ctx, const libconfig::Setting & s);
};

#endif
