#include <boost/math/special_functions/gamma.hpp>
#include <boost/lexical_cast.hpp>
#include <cstdio>

int main(int argc, char ** argv){
    if(argc!=3){
        return 1;
    }
    double n = boost::lexical_cast<double>(argv[1]);
    double q_est = boost::lexical_cast<double>(argv[2]);
    std::printf("%.6f ", boost::math::gamma_p(n + 1, q_est));
    return 0;
}
