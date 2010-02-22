#ifndef PLUGIN_CORE_HPP
#define PLUGIN_CORE_HPP

#include "interface/plugin_so_interface.hpp"
#include "libconfig/libconfig.h++"
#include "plugins/run_plain.hpp"

//HistogramFunction Plugins. "fixed-" prefix is a convention and means that the Histogram
// does not depend on any parameters. In this case, it is suitable to return
// a ConstantHistogramFunction.
class FixedPolyHistoFactory: public theta::plugin::HistogramFunctionFactory{
public:
   virtual std::auto_ptr<theta::HistogramFunction> build(theta::plugin::ConfigurationContext & ctx) const;
   virtual std::string getTypeName() const{
      return "fixed-poly";
   }
};


class FixedGaussHistoFactory: public theta::plugin::HistogramFunctionFactory{
public:
   virtual std::auto_ptr<theta::HistogramFunction> build(theta::plugin::ConfigurationContext & ctx) const;
   virtual std::string getTypeName() const{
      return "fixed-gauss";
   }
};

#endif
