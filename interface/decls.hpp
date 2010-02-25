#ifndef THETA_DECLS_HPP
#define THETA_DECLS_HPP

namespace libconfig{
    class Setting;
}

namespace theta{
    class Minimizer;
    class Producer;
    class Data;
    class Model;
    class VarIdManager;
    
    namespace utils{
        class SettingUsageRecorder;
    }
    
    namespace plugin{}
    
    
template<typename rndsourcetype, unsigned int min, unsigned int max> class RandomAlgos;
class RandomTausSource;
typedef RandomAlgos<RandomTausSource, 0, 0xffffffffL> RandomTaus;


template<typename rndtype> class RunT;
typedef RunT<RandomTaus> Run;

}

#endif
