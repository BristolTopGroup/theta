#include "interface/plugin.hpp"
#include "interface/histogram-function.hpp"
#include "root/root_histogram.hpp"

#include "TH1.h"
#include "TFile.h"

using namespace theta;
using namespace theta::plugin;
using namespace std;

   root_histogram::root_histogram(const Configuration & ctx){
      string filename = ctx.setting["filename"];
      string histoname = ctx.setting["histoname"];
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
      bool use_errors = ctx.setting["use_errors"];
      for(int i=0; i<=nbins+1; i++){
          double content = histo->GetBinContent(i);
          h.set(i, content);
          //h_error contains the relative errors:
          if(use_errors && content > 0.0){
             h_error.set(i, histo->GetBinError(i) / content);
          }
      }
      double norm = HistogramFunctionUtils::read_normalize_to(ctx.setting);
      double integral = h.get_sum_of_bincontents();
      h *= norm/integral;
      set_histos(h, h_error);
   }

REGISTER_PLUGIN(root_histogram)
