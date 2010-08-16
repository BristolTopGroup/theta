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
    int rebin = 1;
    double range_low = -999;
    double range_high = -999;
    if(ctx.setting.exists("rebin")){
       rebin = ctx.setting["rebin"];
    }
    if(ctx.setting.exists("range")){
       range_low = ctx.setting["range"][0];
       range_high = ctx.setting["range"][1];
    }
    
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
    histo->Rebin(rebin);
    double norm = HistogramFunctionUtils::read_normalize_to(ctx.setting);
    histo->Scale(norm / histo->Integral());
    
    int bin_low = 1;
    int bin_high = histo->GetNbinsX();
    if(range_low!=-999){
       bin_low = histo->GetXaxis()->FindBin(range_low);
    }
    if(range_high!=-999){
       bin_high = histo->GetXaxis()->FindBin(range_high);
    }
    int nbins = bin_high - bin_low + 1;
    
    double xmin = histo->GetXaxis()->GetXmin();
    double xmax = histo->GetXaxis()->GetXmax();
    //int nbins = histo->GetNbinsX();
    if(bin_low > 0)
       xmin = histo->GetXaxis()->GetBinLowEdge(bin_low);
    if(bin_high <= nbins)
       xmax = histo->GetXaxis()->GetBinUpEdge(bin_high);
    
    bool use_errors = false;
    if(ctx.setting.exists("use_errors")){
         use_errors = ctx.setting["use_errors"];
    }
    
    theta::Histogram h(nbins, xmin, xmax);
    theta::Histogram h_error(nbins, xmin, xmax);
    for(int i = bin_low; i <= bin_high; i++){
        double content = histo->GetBinContent(i);
        h.set(i - bin_low + 1, content);
        //h_error contains the relative errors:
        if(use_errors && content > 0.0){
           h_error.set(i - bin_low + 1, histo->GetBinError(i) / content);
        }
    }
    set_histos(h, h_error);
}

REGISTER_PLUGIN(root_histogram)

