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
 * <pre>
 * {
 * type = "root_histogram";
 * filename = "path/to/file.root";
 * histoname = "histoname-in-file";
 * normalize_to = 1.0;
 * use_errors = true;
 * }
 * </pre>
 *
 * If \c use_errors is true, the errors in the Histogram will be used to
 * build a \c ConstantHistogramFunctionError instance which implements bin-by-bin
 * fluctuations for pseudodata creation (for deails, see documentation of
 * \c ConstantHistogramFunctionError).
 *
 * Otherwise, an instance of \c ConstantHistogramFunction
 * is returned which does not treat parametrization errors.
 *
 * \sa ConstantHistogramFunctionError ConstantHistogramFunction
 */
class root_histogram: public ConstantHistogramFunctionError{
public:
    /// Constructor used by the plugin system
    root_histogram(const Configuration & ctx);
};
