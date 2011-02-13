#include "interface/histogram.hpp"
#include "interface/random.hpp"
#include "interface/exception.hpp"
#include "interface/utils.hpp"

#include <cmath>
#include <sstream>
#include <limits>
#include <new>

using namespace theta;

namespace{
   double * allocate_histodata(size_t nbins){
      double * result = 0;
      const size_t nbins_orig = nbins;
      //always allocate an even number of bins:
      if(nbins_orig % 2) ++nbins;
      //for the add_fast routine, which ight use SSE optimizations, we need this alignment. And
      // while we at it, we should make sure double is as expected:
      BOOST_STATIC_ASSERT(sizeof(double)==8);
      int err = posix_memalign((void**)(&result), 16, sizeof(double) * (nbins + 2));
      if(err!=0){
        throw std::bad_alloc();
      }
      //set the extra allocated double to zero to make sure no time-consuming garbage is there ...
      if(nbins_orig % 2) result[nbins + 1] = 0.0;
      return result;
   }
   
   void free_histodata(double * histodata){
      free(histodata);
   }
}


Histogram::Histogram(size_t b, double x_min, double x_max) :
sum_of_bincontents(0.0), sum_of_bincontents_valid(true), nbins(b), xmin(x_min), xmax(x_max) {
    if(xmin >= xmax) throw InvalidArgumentException("Histogram: xmin >= xmax not allowed");
    histodata = allocate_histodata(nbins);
    memset(histodata, 0, sizeof (double) *(nbins + 2));
}

Histogram::Histogram(const Histogram& rhs) {
    initFromHisto(rhs);
}

void Histogram::operator=(const Histogram & rhs) {
    if (&rhs == this) return;
    if (nbins != rhs.nbins || xmin != rhs.xmin || xmax != rhs.xmax) {
        free_histodata(histodata);
        initFromHisto(rhs);
    } else {
        memcpy(histodata, rhs.histodata, sizeof (double) *(nbins + 2));
        sum_of_bincontents_valid = rhs.sum_of_bincontents_valid;
        sum_of_bincontents = rhs.sum_of_bincontents;
    }
}

Histogram::~Histogram() {
    free_histodata(histodata);
}

void Histogram::reset(size_t b, double x_min, double x_max) {
    sum_of_bincontents = 0.0;
    sum_of_bincontents_valid = true;
    //only re-allocate if there where changes.
    if (b > 0 && (b != nbins || x_min != xmin || x_max != xmax)) {
        nbins = b;
        xmin = x_min;
        xmax = x_max;
        if(xmin >= xmax) throw InvalidArgumentException("Histogram: xmin >= xmax not allowed");
        free_histodata(histodata);
        histodata = allocate_histodata(nbins);
    }
    memset(histodata, 0, sizeof (double) *(nbins + 2));
}

void Histogram::reset_to_1(){
   for(size_t i=0; i<=nbins+1; i++){
      histodata[i] = 1.0;
   }
   sum_of_bincontents = nbins+2;
   sum_of_bincontents_valid = true;
}

void Histogram::multiply_with_ratio_exponented(const Histogram & nominator, const Histogram & denominator, double exponent){
   check_compatibility(nominator);
   check_compatibility(denominator);
   const double * __restrict n_data = nominator.histodata;
   const double * __restrict d_data = denominator.histodata;
   double s = 0.0;
   for(size_t i=0; i<=nbins+1; i++){
      if(d_data[i]>0.0)
         histodata[i] *= pow(n_data[i] / d_data[i], exponent);
      s += histodata[i];
   }
   sum_of_bincontents = s;
   sum_of_bincontents_valid = true;
}

void Histogram::initFromHisto(const Histogram & h) {
    nbins = h.nbins;
    xmin = h.xmin;
    xmax = h.xmax;
    sum_of_bincontents = h.sum_of_bincontents;
    sum_of_bincontents_valid = h.sum_of_bincontents_valid;
    histodata = allocate_histodata(nbins);
    memcpy(histodata, h.histodata, sizeof (double) *(nbins + 2));
}

void Histogram::fill(double xvalue, double weight) {
    int bin = static_cast<int> ((xvalue - xmin) * nbins / (xmax - xmin) + 1);
    if (bin < 0)
        bin = 0;
    if (static_cast<size_t> (bin) > nbins + 1)
        bin = nbins + 1;
    histodata[bin] += weight;
    if(sum_of_bincontents_valid)
        sum_of_bincontents += weight;
}

void Histogram::fail_check_compatibility(const Histogram & h) const {
    if (nbins != h.nbins || xmin!=h.xmin || xmax != h.xmax){
        std::stringstream s;
        s <<  "Histogram::check_compatibility: Histograms are not compatible (nbins, xmin, xmax) are: "
              " (" << nbins << ", " << xmin << ", " << xmax << ") and "
              " (" << h.nbins << ", " << h.xmin << ", " << h.xmax << ")";
        throw InvalidArgumentException(s.str());
    }
}

void Histogram::operator*=(const Histogram & h) {
    check_compatibility(h);
    const double * hdata = h.histodata;
    sum_of_bincontents = 0.0;
    for (size_t i = 0; i <= nbins + 1; ++i) {
        histodata[i] *= hdata[i];
        sum_of_bincontents += histodata[i];
    }
    sum_of_bincontents_valid = true;
}

