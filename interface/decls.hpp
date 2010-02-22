#ifndef THETA_DECLS_HPP
#define THETA_DECLS_HPP

namespace theta{

template<typename rndsourcetype, unsigned int min, unsigned int max> class RandomAlgos;
class RandomTausSource;
typedef RandomAlgos<RandomTausSource, 0, 0xffffffffL> RandomTaus;


template<typename rndtype> class RunT;
typedef RunT<RandomTaus> Run;

}

#endif
