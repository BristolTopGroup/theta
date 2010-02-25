#include "interface/histogram.hpp"
#include "interface/exception.hpp"
#include "interface/utils.hpp"

//for memcpy and alike:
#include <cstring>
#include <cmath>
#include <limits>

using namespace theta;

Histogram::Histogram(size_t b, double x_min, double x_max) :
sum_of_bincontents(0.0), nbins(b), xmin(x_min), xmax(x_max) {
    histodata = new double[nbins + 2];
    memset(histodata, 0, sizeof (double) *(nbins + 2));
}

Histogram::Histogram(const Histogram& rhs) {
    initFromHisto(rhs);
}

Histogram & Histogram::operator=(const Histogram & rhs) {
    if (&rhs == this) return *this;
    if (nbins != rhs.nbins || xmin != rhs.xmin || xmax != rhs.xmax) {
        delete[] histodata;
        initFromHisto(rhs);
    } else {
        memcpy(histodata, rhs.histodata, sizeof (double) *(nbins + 2));
        sum_of_bincontents = rhs.sum_of_bincontents;
    }
    return *this;
}

Histogram::~Histogram() {
    delete[] histodata;
}

void Histogram::reset(size_t b, double x_min, double x_max) {
    sum_of_bincontents = 0.0;
    //only re-allocate if there where changes.
    if (b != 0 && (b != nbins || x_min != xmin || x_max != xmax)) {
        nbins = b;
        xmin = x_min;
        xmax = x_max;
        delete[] histodata;
        histodata = new double[nbins + 2];
    }
    memset(histodata, 0, sizeof (double) *(nbins + 2));
}

void Histogram::reset_to_1(){
   for(size_t i=0; i<=nbins+1; i++){
      histodata[i] = 1.0;
   }
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
}

void Histogram::add_with_coeff(double coeff, const Histogram & other){
    check_compatibility(other);
    const double * __restrict data = other.histodata;
    for(size_t i=0; i<=nbins+1; i++){
        histodata[i] += coeff * data[i];
    }
    sum_of_bincontents += coeff * other.sum_of_bincontents;
}

void Histogram::initFromHisto(const Histogram & h) {
    nbins = h.nbins;
    xmin = h.xmin;
    xmax = h.xmax;
    sum_of_bincontents = h.sum_of_bincontents;
    histodata = new double[nbins + 2];
    if (h.histodata != 0) {
        memcpy(histodata, h.histodata, sizeof (double) *(nbins + 2));
    }
}

void Histogram::fill(double xvalue, double weight) {
    int bin = static_cast<int> ((xvalue - xmin) * nbins / (xmax - xmin) + 1);
    if (bin < 0)
        bin = 0;
    if (static_cast<size_t> (bin) > nbins + 1)
        bin = nbins + 1;
    histodata[bin] += weight;
    sum_of_bincontents += weight;
}

void Histogram::check_compatibility(const Histogram & h) const {
    if (nbins != h.nbins) throw InvalidArgumentException("Histogram:check_compatibility: number of bins mismatch");
    double binwidth = (xmax - xmin) / nbins;
    if (!utils::close_to(h.xmax, xmax, binwidth) || !utils::close_to(h.xmin, xmin, binwidth))
        throw InvalidArgumentException("Histogram borders mismatch (too much).");
}

Histogram & Histogram::operator+=(const Histogram & h) {
    if(&h == this){
        this->operator*=(2.0);
        return *this;
    }
    check_compatibility(h);
    const double * __restrict hdata = h.histodata;
    for (size_t i = 0; i <= nbins + 1; i++) {
        histodata[i] += hdata[i];
    }
    sum_of_bincontents += h.sum_of_bincontents;
    return *this;
}

Histogram & Histogram::operator*=(const Histogram & h) {
    check_compatibility(h);
    const double * hdata = h.histodata;
    sum_of_bincontents = 0.0;
    for (size_t i = 0; i <= nbins + 1; i++) {
        histodata[i] *= hdata[i];
        sum_of_bincontents += histodata[i];
    }
    return *this;
}


Histogram & Histogram::operator*=(double a) {
    for (size_t i = 0; i <= nbins + 1; i++) {
        histodata[i] *= a;
    }
    sum_of_bincontents *= a;
    return *this;
}

/*double Histogram::get_quantile(double q) const {
    q *= sum_of_bincontents;
    //if quantile under underflow requested: return -inf:
    if (q < histodata[0]) return -std::numeric_limits<double>::infinity();
    if (mcmcutils::close_to(q, sum_of_bincontents - histodata[nbins + 1], sum_of_bincontents)) return xmax;
    if (q > sum_of_bincontents - histodata[nbins + 1]) return std::numeric_limits<double>::infinity();
    double sum = histodata[0];
    size_t bin = 1;
    while (sum <= q && bin <= nbins + 1) {
        sum += histodata[bin++];
    }
    bin--;
    //bin is now the index of the bin in which the quantile is
    // reached.
    if (bin == nbins + 1) {
        //should not happen or our event weight was wrong ...
        // So this only happens for rounding errors and q close to 1.
        return xmax;
    }
    //interpolate last bin linearly:
    double last_bin_fraction = 1 - (sum - q) / histodata[bin];
    return (xmax - xmin) / nbins * (bin - 1 + last_bin_fraction) + xmin;
}*/
