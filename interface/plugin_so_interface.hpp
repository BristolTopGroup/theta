#ifndef PLUGIN_SO_INTERFACE_HPP
#define PLUGIN_SO_INTERFACE_HPP

#include "interface/decls.hpp"
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

    /// The root setting of the configuration file, to resolve any references to other settings
    const libconfig::Setting & rootsetting;

    /// The setting to be used to construct the object
    const libconfig::Setting & setting;

    /// To mark the settings as used (for warning the user if there are unused settings)
    utils::SettingUsageRecorder & rec;

public:
    /** \brief Construct Configuration by specifying all data members
     */
    Configuration(boost::shared_ptr<VarIdManager> & vm_, const libconfig::Setting & rootsetting_, const libconfig::Setting & setting_,
        utils::SettingUsageRecorder & rec_):
        vm(vm_), rootsetting(rootsetting_), setting(setting_), rec(rec_){}

    /** \brief Copy elements from another Configuration, but replace Configuration::setting
     *
     * Copy all from \c cfg but \c cfg.setting which is replaced by \c setting_.
     */
    Configuration(const Configuration & cfg, const libconfig::Setting & setting_): vm(cfg.vm),
        rootsetting(cfg.rootsetting), setting(setting_), rec(cfg.rec){}
};


}}

#endif
