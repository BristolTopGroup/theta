#ifndef HISTOGRAM_HPP
#define HISTOGRAM_HPP

#include <vector>
#include <cmath>
#include <algorithm>
#include "interface/utils.hpp"
#include "interface/exception.hpp"


namespace theta{

/** A Histogram class holding binned data.
 *
 * Only equally spaced binning is supported. Binning is therefore fully determined
 * by a range of double (\c xmin, \c xmax) and the number of bins, \c nbins.
 *
 * Bin index convention:
 * - bin 0 is the "underflow bin", it corresponds to the range (-infinity, xmin)
 * - bin \c i with \c i &gt;=0 and \c i &lt;= nbins holds weight for the range [xmin + (i-1)*binwidth, xmin + i*binwidth), where binwidth = (xmax-xmin)/nbins.
 * - bin nbins+1 is the "overflow bin", it corresponds to the range [xmax, infinity).
 */
class Histogram {
//friend class boost::serialization::access;
private:
    double* histodata;
    double sum_of_weights;
    //number of bins for the histogram, *excluding* underflow and overflow:
    size_t nbins;
    double xmin, xmax;
    
    void initFromHisto(const Histogram & h);
    
public:
    /** Create an empty Histogram with \c bins bins with range (\c xmin, \c xmax ).
    */
    explicit Histogram(size_t bins=0, double xmin=0, double xmax=0);

    /// Copy constructor. Copies the contents of \c rhs into this.
    Histogram(const Histogram& rhs);

    /// Copy assignment. Copies the content of \c rhs into this.
    Histogram & operator=(const Histogram& rhs);

    ///Destructor.
    ~Histogram();

    /** Set all entries to zero.
     *
     * If \c nbins!=0, also change the number of bins and range to the parameters given. Otherwise,
     * binning and range remain unchanged.
     */
    void reset(size_t nbins=0, double xmin=0, double xmax=0);

    /** Set all bin entries to 1.0.
     * This is mainly useful for resetting Histograms subsequently used for multiplication.
     */
    void reset_to_1();

    /** Get the entry for bin \c i.
    */
    double get(size_t i) const{
        return histodata[i];
    }

    /** Set bin entry i to \c weight.
    */
    void set(size_t i, double weight){
       sum_of_weights += weight - histodata[i];
       histodata[i] = weight;
    }

    /** Return a pointer to the raw Histogram data. Note that the binning convention applies, i.e.
     * valid indices for the returned pointer are 0 to nbins+1.
     */
    const double* getData() const{
        return histodata;
    }

    /// Get the number of bins of this Histogram.
    size_t get_nbins() const{
        return nbins;
    }

    /// Get \c xmin for this Histogram.
    double get_xmin() const{
       return xmin;
    }

    // Get \c xmax of this Histogram.
    double get_xmax() const{
       return xmax;
    }

    /** Add \c weight corresponding to the bin of \c xvalue.
     *
     * In case of xvalue &lt; xmin, \c weigt is added to the underflow bin 0.
     * If xvalue &gt; xmax, \c weight is added to bin nbins+1.
     */
    void fill(double xvalue, double weight);

    /// Get the sum of weights of all bins of the Histogram.
    double get_sum_of_weights() const{
        return sum_of_weights;
    }

    /** the x value for the bin center of bin \c ibin.
     *  The returned value is only valid for ibin>=1 && ibin&lt;=nbins
     */
    double get_bincenter(int ibin) const{
        return xmin + (ibin-0.5) * (xmax - xmin) / nbins;
    }

    /** Add the Histogram \c other to this.
     * Throws an \c InvalidArgumentException if \c other is not compatible to this.
     */
    Histogram & operator+=(const Histogram & other);

    /** Multiply the Histogram \c other with this, bin-by-bin.
     * Throws an \c InvalidArgumentException if \c other is not compatible with this Histogram.
     * \c &other must be different from \c this.
     */
    Histogram & operator*=(const Histogram & other);

    /// Multiply the entry of each bin by \c a.
    Histogram & operator*=(const double a);

    /** check compatibility of \c this to the \c other Histogram. A Histogram is considered compatible
     * if it has the exact same number of bins and range. In case if incompatibility, an 
     * \c InvalidArgumentException is thrown.*/
    void check_compatibility(const Histogram & h) const;
    
    
    /** Calculate this = this * (nominator/denominator)^exponent, bin-by-bin.
    * An \c InvalidArgumentException is thrown if either \c nominator or \c denominator are not compatible with this.
    */
    void multiply_with_ratio_exponented(const Histogram & nominator, const Histogram & denominator, double exponent);

    /** Calculate this = this + coeff * other.
    * An \c InvalidArgumentException is thrown if \c other is not compatible with this.
    */
    void add_with_coeff(double coeff, const Histogram & other);
    
    //double get_quantile(double q) const;

    /** Use the current Histogram as pdf to draw random numbers and fill them into \c m.
     * 
     * \c m will be reset to match the range and number of bins of the current Histogram.
     * \c rnd is the random number generator to use
     * \c mu is either the exact number of points to sample or the mean of a Poisson to use to determine the number of sample points. 
     *  If \c mu &lt; 0, the current \c sum_of_weigths is used as \c mu.
     * If \c use_poisson is false, mu is rounded and used as the number of created entries. Otherwise, \c m will contain 
     * a total weight distributed according to a Poisson with mean \c mu.
     */ 
    template<class rndtype>
    void fill_with_pseudodata(Histogram & m, rndtype & rnd, double mu=-1, bool use_poisson=true) const;
};


/*TODO: the complexity is not optimal: it is O(N*log(N) + M) for a sample size N and M bins. Also, space requirement is N*sizeof(double).
 * Maybe it would be better to use a simple N*M algo.
 * It would also be possible to do it in N*log(M), or maybe O(M + sqrt(N))
 *
 * If use_poisson is true, a O(M) complexity algorithm is used.
 */
template<class rndtype>
void Histogram::fill_with_pseudodata(Histogram & m, rndtype & rnd, double mu, bool use_poisson) const{
    m.reset(nbins, xmin, xmax);
    if(mu<0) mu = sum_of_weights;
    if(!use_poisson){
        const size_t N = static_cast<size_t>(mu+0.5);
        std::vector<double> r(N);
        for(size_t i=0; i<N; i++){
            r[i] = rnd.uniform();
        }
        std::sort(r.begin(), r.end());
        //normalized integral up to and including bin:
        double integral = 0;
        //next r to probe:
        size_t next_r = 0;
        for(size_t bin=0; bin<=nbins+1; bin++){
            integral += histodata[bin]/sum_of_weights;
            size_t n=0;
            while(next_r < N && r[next_r] <= integral){
                n++;
                next_r++;
            }
            m.set(bin, n);
        }
    }
    else{//use_poisson.
        double factor = mu/sum_of_weights;
        for(size_t bin=0; bin<=nbins+1; bin++){
            size_t n = rnd.poisson(factor * histodata[bin]);
            m.set(bin, n);
        }
    }
}

}

#endif

