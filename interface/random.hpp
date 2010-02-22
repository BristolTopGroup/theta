#ifndef RANDOM_H
#define RANDOM_H

#include <cmath>
#include "interface/exception.hpp"
#include "interface/utils.hpp"
#include "interface/decls.hpp"


namespace theta {

/* \file Random number generators are divided in two parts:
 *  1. the actual random number generator, which derives from RandomSource
 *      and only provides very few methods to generate random numbers uniformly.
 *  2. A RandomAlgos class which takes a RandomSource as template argument
 *    and provides more random number distributions.
 * 
 * Using templates instead of inheritance (where each generator overrides polymorphically the
 * uniform generation routine) makes things faster, as no vtable redirections are necessary.
 * 
 * An end user will almost certainly use a RandomAlgos class. Note that
 * such pre-templated classes are available as typedefs.
 */

template<typename t, unsigned int, unsigned int> class RandomAlgos;

//in order to prevent that every template instantiaion of
// RandomAlgos is polluted with the same helper data for
// the random distribution's algorithms,
// common static data is saved here:
class RandomAlgosData {
//all RandomAlgos are our friend:
template<typename, unsigned int, unsigned int> friend class RandomAlgos;
private:
    /* The Gauss Zigurrat method. */
    static const double ytab[128];
    /* tabulated values for 2^24 times x[i]/x[i+1],
     * used to accept for U*x[i+1]<=x[i] without any floating point operations */
    static const long int ktab[128];
    /* tabulated values of 2^{-24}*x[i] */
    static const double wtab[128];
};

//some random algorithms which can be applied to all random sources.
//a random source delivers uniform() distributions and a random integer
// between min and max. As min and max are known at compile time,
// this can be used to optimize run time and compile-time error checking.
template<typename rndsourcetype, unsigned int min, unsigned int max>
class RandomAlgos {
private:
    rndsourcetype rnd;

    double gauss_zig(double)throw();
    unsigned int poisson_root(double mean)throw();
    unsigned int get_uniform_int(unsigned int n)throw();
public:
    RandomAlgos(){}
    RandomAlgos(const RandomAlgos<rndsourcetype, min, max> & rhs):rnd(rhs.rnd){
    }
    inline double gauss(double sigma = 1.0)throw(){
        return gauss_zig(sigma);
    }
    inline unsigned int poisson(double mean)throw(){
        return poisson_root(mean);
    }
    inline double uniform()throw(){
        return rnd.uniform();
    }
    inline void setSeed(unsigned long int seed){
        rnd.setSeed(seed);
    }
    inline unsigned int get(){
       return rnd.get();
    }
};

template<typename rndsourcetype, unsigned int min, unsigned int max>
double RandomAlgos<rndsourcetype, min, max>::gauss_zig(double sigma) throw(){
        long int j;
        int i;
        int sign;
        double x, y;
        const double PARAM_R = 3.44428647676;
        const unsigned int range = max - min;
        //const long int offset = min;
        while (true) {
            if (range >= 0xFFFFFFFFL) {
                long k = rnd.get() - min;
                i = static_cast<int>(k & 0xFF);
                j = (k >> 8) & 0xFFFFFFL;
            } else if (range >= 0x00FFFFFFL) {
                long k1 = rnd.get() - min;
                long k2 = rnd.get() - min;
                i = static_cast<int>(k1 & 0xFF);
                j = (k2 & 0x00FFFFFF);
            } else {
                i = static_cast<int>(get_uniform_int(256)); /*  choose the step */
                j = get_uniform_int(16777216);  /* sample from 2^24 */
            }

            sign = ((i & 0x80)!=0) ? +1 : -1;
            i &= 0x7f;

            x = j * RandomAlgosData::wtab[i];

            if (j < RandomAlgosData::ktab[i]) {
                break;
            }
            if (i < 127) {
                double y0, y1, U1;
                y0 = RandomAlgosData::ytab[i];
                y1 = RandomAlgosData::ytab[i + 1];
                U1 = rnd.uniform();
                y = y1 + (y0 - y1) * U1;
            } else {
                double U1, U2;
                U1 = 1.0 - rnd.uniform();
                U2 = rnd.uniform();
                x = PARAM_R - log(U1) / PARAM_R;
                y = exp(-PARAM_R * (x - 0.5 * PARAM_R)) * U2;
            }
            if (y < exp(-0.5 * x * x)) {
                break;
            }
        }
        return sign * sigma * x;
    }

template<typename rndsourcetype, unsigned int min, unsigned int max>
unsigned int RandomAlgos<rndsourcetype, min, max>::poisson_root(double mean)throw() {
    unsigned int n;
    if (mean <= 0) return 0;
    if (mean < 25) {
        double expmean = exp(-mean);
        double pir = 1;
        n = 0;
        while (1) {
            n++;
            pir *= rnd.uniform();
            if (pir <= expmean) break;
        }
        return n - 1;
    }// for large value we use inversion method
        else if (mean < 1E9) {
            double em, t, y;
            double sq, alxm, g;
            //double pi = M_PI;
            sq = sqrt(2.0 * mean);
            alxm = log(mean);
            g = mean * alxm - utils::lngamma(mean + 1.0);
            do {
                do {
                    y = tan(M_PI * rnd.uniform());
                    em = sq * y + mean;
                } while (em < 0.0);
                em = floor(em);
                t = 0.9 * (1.0 + y * y) * exp(em * alxm - utils::lngamma(em + 1.0) - g);
            } while (rnd.uniform() > t);
            return static_cast<unsigned int> (em);
        } else {
            // use Gaussian approximation vor very large values
            n = static_cast<unsigned int> (gauss(sqrt(mean)) + mean + 0.5);
            return n;
        }
    }

    template<typename rndsourcetype, unsigned int min, unsigned int max>
    unsigned int RandomAlgos<rndsourcetype, min, max>::get_uniform_int(unsigned int n) throw() {
        unsigned int offset = min;
        unsigned int range = max - min;
        unsigned int scale;
        unsigned int k;
        scale = range / n;
        do {
            k = (rnd.get() - offset) / scale;
        } while (k >= n);
        return k;
    }

    //making the function calls inline makes gauss_zig() about 40% faster ...
    class RandomTausSource{
    private:
        unsigned int s1, s2, s3;
    public:
        RandomTausSource();
        inline double uniform() throw(){
            return get() / 4294967296.0;
        }
        inline unsigned int get() throw(){
            s1 = ((s1 & 0xFFFFFFFE) << 12) ^ (((s1 << 13)^s1) >> 19);
            s2 = ((s2 & 0xFFFFFFF8) << 4)  ^ (((s2 << 2)^s2) >> 25);
            s3 = ((s3 & 0xFFFFFFF0) << 17) ^ (((s3 << 3)^s3) >> 11);
            return s1 ^ s2 ^ s3;
        }
        inline unsigned int min() const throw(){
            return 0;
        }
        inline unsigned int max() const throw(){
            return 0xffffffffL;
        }
        void setSeed(unsigned long int);
    };

//    typedef RandomAlgos<RandomTausSource, 0, 0xffffffffL> RandomTaus;

    /** A proxy class for random number generators to allow non-template usage of random number generators.
     *
     * The original design was to template everything that uses random number generators for high performance. This produces
     * a lot of classes which need also templating: for example, Distribution has to be templated, because
     * it contains virtual methods which use a random number generator (it is not possible to template virtual methods
     * directly, hence the whole class needs templating).
     * This however also requires eveything that uses distributions to be templated, like Model. This
     * is not feasable. Therefore, introduce one layer of redirection, getting rid of templating, i.e. allowing that
     * classes which use random number generators use polymorphic random class instead of being templated.
     *
     * In this solution, templated versions (which circumvent any redirection) can co-exist.
     */
    class AbsRandomProxy{ //"Abs" for abstract. Not really a good name ...
    public:
    virtual double gauss(double sigma = 1.0) = 0;
    virtual unsigned int poisson(double mean) = 0;
    virtual double uniform() = 0;
    virtual void setSeed(unsigned long int seed)=0;
    virtual unsigned int get() = 0;
    virtual ~AbsRandomProxy(){}
    };

    /** The concrete proxy to a RandomAlgos<...>-like interface.
     *
     * Note that it is a proxy to a class which must be valid longer than to proxy object.
     */
    template<typename rndtype>
    class RandomProxy: public AbsRandomProxy{
    public:
        RandomProxy(rndtype & rnd_): rnd(rnd_){}
        virtual ~RandomProxy(){}
        virtual double gauss(double sigma = 1.0){
            return rnd.gauss(1.0);
        }
        virtual unsigned int poisson(double mean){
            return rnd.poisson(mean);
        }
        virtual double uniform(){
            return rnd.uniform();
        }
        virtual void setSeed(unsigned long int seed){
            rnd.setSeed(seed);
        }
        virtual unsigned int get(){
            return rnd.get();
        }
    private:
        rndtype & rnd;
    };
}


#endif
