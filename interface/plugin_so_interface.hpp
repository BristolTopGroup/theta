#ifndef PLUGIN_SO_INTERFACE_HPP
#define PLUGIN_SO_INTERFACE_HPP

#include "interface/decls.hpp"
#include <boost/shared_ptr.hpp>

/**
 * \file plugin_so_interface.hpp
 * This header file defines the interface for plugins and is for inclusion by plugins.
 */
namespace theta{
    namespace plugin{


/** \brief A container class to pass common elements to the factories/plugins
 */
class Configuration{
public:
    /// Information about all currently known parameters and observables:
    boost::shared_ptr<VarIdManager> vm;

    /// The root setting of the configuration file, to resolve any references to other settings:
    const libconfig::Setting & rootsetting;

    /// The setting to be used to construct the object:
    const libconfig::Setting & setting;

    /// To mark the settings as used (for warning the user if there are unused settings):
    utils::SettingUsageRecorder & rec;

public:
    Configuration(boost::shared_ptr<VarIdManager> & vm_, const libconfig::Setting & rootsetting_, const libconfig::Setting & setting_,
        utils::SettingUsageRecorder & rec_):
        vm(vm_), rootsetting(rootsetting_), setting(setting_), rec(rec_){}

    /** \brief Pseudo copy-constructor.
     *
     * Copy all from \cfg but setting.
     */
    Configuration(const Configuration & cfg, const libconfig::Setting & setting_): vm(cfg.vm),
        rootsetting(cfg.rootsetting), setting(setting_), rec(cfg.rec){}
};


/** \brief An externally provided factory producing various configurable components of the %theta framework.
 * 
 *  A "Plugin" class is a factory which produces certain kind of products.
 * There can be many Plugin instances for one type of product, each responsible for a different "type"
 * of product.
 * The type name is unique and corresponds to the type="..." Setting in the config file.
 *
 * Example: If a user wants to use a different prior for a model (say, a bi-gaussian),
 *   the user can:
 * <ol>
 *  <li>implement the bi-gaussian as a new class derived from Distribution.</li>
 *  <li>Write a DistributionPlugin (which is a typedef for Plugin<Distribution>), which can construct an instance
 *    of the new class from a <tt>Configuration</tt>. DistributionPlugin::getTypeName must return a
 *    unique name which is human-readable. In this case, one can use "bi-gaussian".</tt>
 *  <li>Compile a module containing 1. and 2., implementing also the appropriate plugin initialization function
 *    (see get_*_plugins below) into a shared object (say "plugin_bigaussian.so").</li>
 *  <li>In a theta config file, include the path to plugin_bigaussian.so in the plugins = {}; setting.</li>
 * </ol>
 * After these steps, the Distribution can be configured and used just like any "native" distribution of theta.
 */
/*template<typename product>
class Factory{
public:
    typedef product product_type;
    virtual std::auto_ptr<product> build(Configuration & ctx) const = 0;
    virtual std::string getTypeName() const = 0;
};

typedef Factory<HistogramFunction> HistogramFunctionFactory;
typedef Factory<Run> RunFactory;
typedef Factory<Function> FunctionFactory;
typedef Factory<Minimizer> MinimizerFactory;
typedef Factory<Producer> ProducerFactory;
typedef Factory<Distribution> DistributionFactory;*/


// forward declaration for register_factory:
//template<typename factory_type> class PluginManager;

/** \brief Dummy data structure for plugin registration.
 *
 * Creating an instance of this struct will register the
 * given factory.
 */
/*template<typename factory_type, typename factory_basetype = Factory<typename factory_type::product_type > >
struct register_factory{
    register_factory(){
        //get the signleton PluginManager instance for the basetype:
        PluginManager<factory_basetype> * fac = PluginManager<factory_basetype>::get_instance();
        std::auto_ptr<factory_basetype> ptr(new factory_type());
        fac->register_factory(ptr);
    }
};*/


}}//namespace theta::plugin

/** \brief macro to be called for any factory.
 *
 * This macro merely defines an instance of register_factory which will
 * create an instance of the given factory class (wit the default constructor of the
 * new factory type) and register it at the central registry.
 *
 * As it is not valid to register a plugin multiple times, you have to make sure that
 * REGISTER_FACTORY is called only once. It is recommended to put it into the \c .cpp file
 * which contains the implementation of your Factory (rather than a header or some other source file).
 *
 * @param factory_type is the class of the new factory. It must inherit from Factory &lt; factory_type::product_type &gt; and be default-constructible.
 */
//the use of an anonymous namespace makes sure that definitions in different files do not conflict.
// In order to make sure they do not conflict within one file, append the line number to the variables name, i.e.,
// it is not allowed to call this macro twice on the same line, but that should not be a problem ...
//#define REGISTER_FACTORY(factory_type) namespace{ static theta::plugin::register_factory<factory_type> CONCAT(f, __LINE__); }

//helper macros for REGISTER_FACTORY
//#define CONCAT(a,b) CONCAT2(a,b)
//#define CONCAT2(a,b) a ## b

#endif	/* PLUGIN_SO_INTERFACE_HPP */
