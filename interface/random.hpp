#ifndef RANDOM_HPP
#define RANDOM_HPP

//#include "interface/exception.hpp"
#include "interface/decls.hpp"

#include <vector>
#include <memory>
#include <boost/utility.hpp>

namespace theta {

/** \file random2.hpp Random number generators are divided in two parts:
 * <ol>
 *  <li>The actual random number generator, which derives from RandomSource
 *      and only provides very few methods to generate uniform random numbers</li>
 *  <li>A RandomAlgos class which has a RandomSource member and provides
 *      more random number distributions.</li>
 * </ol>
 *
 * An end user will almost certainly use a RandomAlgos class. Note that
 * such pre-templated classes are available as typedefs.
 */

/// abstract base class for pseudo random number generators
class RandomSource{
    friend class Random;
    protected:
        /** \brief Fill the buffer with 32 bit random numbers.
         */
        virtual void fill(std::vector<unsigned int> & buffer) throw() = 0;
        
        /** \brief Set the seed of the generator.
         */
        virtual void set_seed(unsigned int seed) = 0;
};

/// class using an internal RandomSource to produce various common distributions
class Random: private boost::noncopyable {
private:
    /* The Gauss Zigurrat method. */
    static const double ytab[128];
    /* tabulated values for 2^24 times x[i]/x[i+1],
    * used to accept for U*x[i+1]<=x[i] without any floating point operations */
    static const long int ktab[128];
    /* tabulated values of 2^{-24}*x[i] */
    static const double wtab[128];
    
    std::auto_ptr<RandomSource> rnd;
    
    //cached random data:
    std::vector<unsigned int> random_data;
    std::vector<unsigned int>::const_iterator current;
    std::vector<unsigned int>::const_iterator end;

    double gauss_zig(double)throw();
    unsigned int poisson_root(double mean)throw();
    unsigned int get_uniform_int(unsigned int n)throw();
    void fill() throw(){
        rnd->fill(random_data);
        current = random_data.begin();
    }
public:
    
    /** \brief Construct a generator with \c rnd_ as underlying source
     *
     * Note that RandomAlgos will take ownership of rnd_.
     */
    //While larger buffer sizes help very much for small buffers, tests have shown that
    // choosing a buffer sizes larger than around 100 does not increase performance significantly.
    Random(RandomSource * rnd_): rnd(rnd_), random_data(100),
        current(random_data.end()), end(random_data.end()){
    }
    
    /** \brief get a random number distributed according to a normal distribution with the given standard deviation
     *
     * Uses the GNU Scientific Library "zigurrat" method. For details and references, see there.
     */
    double gauss(double sigma = 1.0)throw(){
        return gauss_zig(sigma);
    }
    
    /** \brief get a random number distributed according to a poisson distribution
     *
     * Uses the GNU Scientific Library "root" method. For details and references, see there.
     */
    unsigned int poisson(double mean)throw(){
        return poisson_root(mean);
    }
    
    /** \brief get a 32 bit random number from the generator
     */
    unsigned int get() throw(){
       if(current==end) fill();
       return *(current++);
    }
    
    /// returns a uniform random number on [0,1)
    double uniform() throw(){
        return get() / 4294967296.0;
    }
    
    /** \brief set the random number generator seed
     *
     * This calls RandomSource::set_seed
     */
    void set_seed(unsigned int n){
        rnd->set_seed(n);
        //invalidate the buffer:
        current = end;
    }
};


    /** \brief Tausworthe generator
     */
    class RandomSourceTaus: public RandomSource{
    private:
        unsigned int s1, s2, s3;
    protected:
        //@{
        /** \brief Implement the pure virtual methods from RandomSource
         */
        virtual void fill(std::vector<unsigned int> & buffer) throw();
        virtual void set_seed(unsigned int);
        //@}
    public:
        /// Default constructor; uses the same seed each time
        RandomSourceTaus();
    };

    class RandomSourceMersenneTwister: public RandomSource{
    private:
        static const int N = 624;
        static const int M = 397;
        static const unsigned int MATRIX_A = 0x9908b0dfU;
        static const unsigned int UPPER_MASK = 0x80000000U;
        static const unsigned int LOWER_MASK = 0x7fffffffU;
        unsigned int mag01[2];//={0x0U, MATRIX_A};
        /* mag01[x] = x * MATRIX_A  for x=0,1 */
        unsigned long mt[N];
        int mti;
    protected:
        //@{
        /** \brief Implement the pure virtual methods from RandomSource
         */
        virtual void fill(std::vector<unsigned int> & buffer) throw();
        virtual void set_seed(unsigned int);
        //@}
    public:
        /// Default constructor; uses the same seed each time
        RandomSourceMersenneTwister();

    };
}


#endif
