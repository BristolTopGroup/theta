#include "interface/plugin_so_interface.hpp"
#include "interface/histogram-function.hpp"
#include "TH1.h"
#include "TFile.h"

using namespace theta;
using namespace theta::plugin;
using namespace std;


/** \brief Factory to read a Histogram from a root file.
 *
 * Configuration: anywhere, where a (constant) Histogram has to be defined,
 * use a setting like:
 * <pre>
 * {
 * type = "root-histogram";
 * filename = "path/to/file.root";
 * histoname = "hitoname-in-file";
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
class RootHistogramFactory: public HistogramFunctionFactory{
public:
   virtual string getTypeName() const {
      return "root-histogram";
   }
   auto_ptr<HistogramFunction> build(ConfigurationContext & ctx) const {
      string filename = ctx.setting["filename"];
      ctx.rec.markAsUsed(ctx.setting["filename"]);
      string histoname = ctx.setting["histoname"];
      ctx.rec.markAsUsed(ctx.setting["histoname"]);
      TFile file(filename.c_str(), "read");
      if(file.IsZombie()){
          stringstream s;
          s << "Could not open file '" << filename << "'";
          throw ConfigurationException(s.str());
      }
      TH1* histo = dynamic_cast<TH1*>(file.Get(histoname.c_str()));
      if(not histo){
         stringstream s;
         s << "Did not find Histogram '" << histoname << "'";
         throw ConfigurationException(s.str());
      }
      double xmin = histo->GetXaxis()->GetXmin();
      double xmax = histo->GetXaxis()->GetXmax();
      int nbins = histo->GetNbinsX();
      theta::Histogram h(nbins, xmin, xmax);
      theta::Histogram h_error(nbins, xmin, xmax);
      for(int i=0; i<=nbins+1; i++){
          double content = histo->GetBinContent(i);
          h.set(i, content);
          //h_error contains the relative errors:
          if(content > 0.0){
             h_error.set(i, histo->GetBinError(i) / content);
          }
      }
      double norm = ctx.setting["normalize_to"];
      ctx.rec.markAsUsed(ctx.setting["normalize_to"]);
      double integral = h.get_sum_of_weights();
      h *= norm/integral;
      bool use_errors = ctx.setting["use_errors"];
      ctx.rec.markAsUsed(ctx.setting["use_errors"]);
      if(use_errors){
          return std::auto_ptr<HistogramFunction>(new ConstantHistogramFunctionError(h, h_error));
      }
      else{
          return std::auto_ptr<HistogramFunction>(new ConstantHistogramFunction(h));
      }
   }
};


void get_histogram_function_factories(boost::ptr_vector<theta::plugin::HistogramFunctionFactory> & factories){
    factories.push_back(new RootHistogramFactory());
}

