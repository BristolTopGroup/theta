#include "interface/plugin.hpp"
#include "interface/histogram-function.hpp"
#include "TH1.h"
#include "TFile.h"

using namespace theta;
using namespace theta::plugin;
using namespace std;

/** \brief Plugin to read Histogram from a root file
 *
 * Configuration: anywhere, where a (constant) Histogram has to be defined,
 * use a setting like:
 * \code
 * {
 * type = "root_histogram";
 * filename = "path/to/file.root";
 * histoname = "histoname-in-file";
 * normalize_to = 1.0; //optional; default is not to scale th histogram
 * use_errors = true; // optional; default is false
 * rebin = 2; // optional; default is 1.
 * range = (0.0, 50.); // optional; default is to use the whole range
 * }
 * \endcode
 *
 * Returns a ConstantHistogramFuntion which contains the specified ROOT histogram from the
 * given root file. Underflow and overflow are usually not copied, unless the range setting (see below)
 * explicitely includes them.
 *
 * If \c use_errors is true, the errors in the Histogram will be used to
 * build a \c ConstantHistogramFunctionError instance which implements bin-by-bin
 * fluctuations for pseudodata creation (for details, see documentation of
 * \c ConstantHistogramFunctionError). Otherwise, an instance of \c ConstantHistogramFunction
 * is returned which does not treat parametrization errors.
 *
 * If rebin is given, TH!::Rebin will be called with this value.
 *
 * If range is given, only bins within this range are copied, not the whole histogram.
 * This can also be used to explicitely include the overflow or underflow bins by specifying
 * a range which goes beyond the border of the histogram. The range will include the bin which contains the
 * lower value up to and including the bin which contains the second value of the bin. This might include
 * more bins that you originally wanted: if you have a ROOT TH1 with 10 bins from 0 to 100 and specify
 * a range (0, 50.0), the bin including the value 50.0 is included, so the actual range will be up to 60.0.
 *
 * Note that normalize_to is applied first to the histogram excluding under / overflow, then the rebinning is done, and then
 * the range setting is applied.
 *
 * \sa ConstantHistogramFunctionError ConstantHistogramFunction
 */
class root_histogram: public ConstantHistogramFunctionError{
public:
    /// Constructor used by the plugin system
    root_histogram(const Configuration & ctx);
};
