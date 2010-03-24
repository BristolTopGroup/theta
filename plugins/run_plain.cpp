#include "run_plain.hpp"
#include "interface/histogram.hpp"

using namespace theta;
using namespace theta::plugin;
using namespace libconfig;
using namespace std;

void plain_run::run_impl() {

}

plain_run::plain_run(const Configuration & cfg): Run(cfg){
}

REGISTER_PLUGIN(plain_run)
