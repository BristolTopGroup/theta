#include "interface/plugin.hpp"
#include "interface/random.hpp"
#include <string>

namespace theta{

/// \brief Base class for plugins using a random number generator.
class RandomConsumer{
protected:
   RandomConsumer(const theta::plugin::Configuration & cfg, const std::string & name);
   int seed;
   std::auto_ptr<Random> rnd_gen;
};


}

