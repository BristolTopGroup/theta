#include "interface/random-utils.hpp"
#include "interface/run.hpp"
#include <boost/date_time/local_time/local_time.hpp>

using namespace theta;

RandomConsumer::RandomConsumer(const theta::plugin::Configuration & cfg, const std::string & name): seed(-1){
   std::auto_ptr<RandomSource> rnd_source;
   std::string source_type = "taus";
   if(cfg.setting.exists("rnd_gen")){
       SettingWrapper s = cfg.setting["rnd_gen"];
       if(s.exists("source_type")){
           source_type = static_cast<std::string>(s["source_type"]);
       }
       if(s.exists("seed")){
          seed = s["seed"];
       }
   }
   if(source_type=="taus"){
      rnd_source.reset(new RandomSourceTaus());
   }
   else if(source_type == "mt"){
      rnd_source.reset(new RandomSourceMersenneTwister());
   }
   else{
      throw ConfigurationException("unknown source_type given for rnd_gen (valid values are 'taus' and 'mt')");
   }
   if(seed == -1){
       using namespace boost::posix_time;
       using namespace boost::gregorian;
       ptime t(microsec_clock::universal_time());
       time_duration td = t - ptime(date(1970, 1, 1));
       seed = td.total_microseconds();
   }
   rnd_gen.reset(new Random(rnd_source.release()));
   rnd_gen->set_seed(seed);
   if(cfg.run){
      cfg.run->get_rndinfo_table().append(*cfg.run, name, seed);
   }
}
         
         
void theta::randomize_poisson(Histogram & h, Random & rnd){
    const size_t nbins = h.get_nbins();
    for(size_t bin=0; bin<=nbins+1; ++bin){
        size_t n = rnd.poisson(h.get(bin));
        h.set(bin, n);
    }
}
                   

