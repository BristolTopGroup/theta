#ifndef PLUGIN_SO_INTERFACE_HPP
#define PLUGIN_SO_INTERFACE_HPP

#include "interface/decls.hpp"
#include "interface/cfg-utils.hpp"
#include <boost/shared_ptr.hpp>

namespace theta{
    namespace plugin{

/** \brief A container class which is used to construct conrete types managed by the plugin system
 *
 * An instance of this class is passed to the constructor of classes managed by the plugin system.
 * It contains the required information for plugins to construct an instance of the plugin class,
 * the most important being \c setting, which is the setting group from the configuration file
 * for which this plugin class should be created.
 */
class Configuration{
public:
    /// Information about all currently known parameters and observables
    boost::shared_ptr<VarIdManager> vm;
    
    /// The setting in the configuration file from which to build the instance
    SettingWrapper setting;
    
public:
    /** \brief Construct Configuration by specifying all data members
     */
    Configuration(const boost::shared_ptr<VarIdManager> & vm_, const SettingWrapper setting_): vm(vm_),
        setting(setting_){}

    /** \brief Copy elements from another Configuration, but replace Configuration::setting
     *
     * Copy all from \c cfg but \c cfg.setting which is replaced by \c setting_.
     */
    Configuration(const Configuration & cfg, const SettingWrapper setting_): vm(cfg.vm),
        setting(setting_){}
};


}}

#endif
