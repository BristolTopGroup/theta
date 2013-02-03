#include <boost/test/unit_test.hpp>
#include <iostream>

#include <stdint.h>
#include <math.h>

#include "interface/log2_dot.hpp"
#include "interface/histogram.hpp"
#include "test/utils.hpp"

using namespace std;
using namespace theta;

#ifdef USE_AMDLIBM
#include "amdlibm/include/amdlibm.h"
#endif

#ifdef __SSE2__
#include <emmintrin.h>
#endif

namespace {

double template_nllikelihood_reference(const double * data, const double * pred, unsigned int n){
   double result = 0.0;
   double pred_norm = 0.0;
   for(unsigned int i=0; i<n; ++i){
        pred_norm += pred[i];
        if(pred[i] > 0.0){
             if(data[i] > 0.0){
                 result -= data[i] * log(pred[i]);
             }
         }else if(data[i] > 0.0){
             return std::numeric_limits<double>::infinity();
         }
    }
    return result + pred_norm;
}

#ifdef USE_AMDLIBM
double template_nllikelihood_amdlibm(const double * data, const double * pred, unsigned int n){
	double result = 0.0;
	double pred_norm = 0.0;
    //double pred_norm = 0.0;
    const unsigned int imax = n/2;
    if(imax > 0){
		__m128d Xpred_norm = _mm_setzero_pd();
		__m128d Xresult = _mm_setzero_pd();
		for(unsigned int i=0; i<imax; ++i){
			// pred_norm += pred[i]:
			__m128d Xp = _mm_load_pd(pred);
			Xpred_norm = _mm_add_pd(Xpred_norm, Xp);

			// if(pred[i] > 0.0 && data[i] > 0.0):

			bool done = false;
			double cmp[2];
			_mm_storeu_pd(cmp, _mm_cmpgt_pd(Xp, _mm_setzero_pd()));
			if(cmp[0] && cmp[1]){
				__m128d Xd = _mm_load_pd(data);
				_mm_storeu_pd(cmp, _mm_cmpgt_pd(Xd, _mm_setzero_pd()));
				if(cmp[0] && cmp[1]){
					// result -= data[i] * log(pred[i]):
					done = true;
					Xp = amd_vrd2_log(Xp);
					Xd = _mm_mul_pd(Xd, Xp);
					Xresult = _mm_sub_pd(Xresult, Xd);
					data += 2;
					pred += 2;
				}
			}
			if(!done){
				// do the two separately, handling all special cases one-by-one:
				for(int k=0; k<2; ++k){
					if(*pred > 0.0){
						if(*data > 0.0){
							result -= *data * log(*pred);
						}else if(*data > 0.0){
							return std::numeric_limits<double>::infinity();
						}
					}
					data += 1;
					pred += 1;
				}
			}
		}

		// fill pred_norm and result from Xpred_norm and Xresult:
		double low, high;
		_mm_storel_pd(&low, Xpred_norm);
		_mm_storeh_pd(&high, Xpred_norm);
		pred_norm += low + high;
		_mm_storel_pd(&low, Xresult);
		_mm_storeh_pd(&high, Xresult);
		result += low + high;
    }


    // last bin separately:
    if(n%2 > 0){
    	pred_norm += *pred;
		if(*pred > 0.0){
			if(*data > 0.0){
				result -= *data * log(*pred);
			}
		}else if(*data > 0.0){
			return std::numeric_limits<double>::infinity();
		}
    }
    return result + pred_norm;
}


#endif


}


BOOST_AUTO_TEST_SUITE(log2_dot_tests)


BOOST_AUTO_TEST_CASE(tl){
    unsigned const int N=5;
    double data[N];
    double pred[N];

    for(int i=0; i<5; i++){
       data[i] = i;
       pred[i] = i + 0.2;
    }
    double ref = template_nllikelihood_reference(data, pred, N);
    double res = template_nllikelihood(data, pred, N);
    BOOST_CHECK(close_to(ref, res, 1.0));
    
    //check that inf is returned if pred is 0.0:
    pred[N/2] = 0.0;
    res = template_nllikelihood(data, pred, N);
    BOOST_CHECK(std::isinf(res) && res > 0);
    
    //check that bin is skipped if data is 0.0:
    data[N/2] = 0.0;
    ref = template_nllikelihood_reference(data, pred, N);
    res = template_nllikelihood(data, pred, N);
    BOOST_REQUIRE(!std::isinf(res) && !std::isinf(ref));
    BOOST_CHECK(close_to(ref, res, 1.0));
}

BOOST_AUTO_TEST_CASE(amdlibm_nll){
	for(unsigned ioff=1; ioff<3; ++ioff){
		for(unsigned int N=5; N <=6; ++N){
			cout << ioff << " " << N << endl;
			DoubleVector h_data(N), h_pred(N);
			double * data = h_data.get_data();
			double * pred = h_pred.get_data();

			for(unsigned int i=0; i<N; i++){
			   data[i] = i;
			   pred[i] = i + 0.2;
			}
			double ref = template_nllikelihood_reference(data, pred, N);
			double res = template_nllikelihood_amdlibm(data, pred, N);
			BOOST_CHECK(close_to(ref, res, 1.0));

			//check that inf is returned if pred is 0.0:
			pred[N-ioff] = 0.0;
			res = template_nllikelihood_amdlibm(data, pred, N);
			BOOST_CHECK(std::isinf(res) && res > 0);

			data[N-ioff] = 0.0;
			ref = template_nllikelihood_reference(data, pred, N);
			res = template_nllikelihood_amdlibm(data, pred, N);

			BOOST_REQUIRE(!std::isinf(res) && !std::isinf(ref));
			BOOST_CHECK(close_to(ref, res, 1.0));
		}
	}
}


BOOST_AUTO_TEST_SUITE_END()
