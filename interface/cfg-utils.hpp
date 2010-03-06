#ifndef CFG_UTILS_HPP
#define	CFG_UTILS_HPP

#include "libconfig/libconfig.h++"
#include "interface/exception.hpp"

#include <string>
#include <set>
#include <vector>
#include <sstream>


namespace theta{

    /** \brief A class to keep record of the used configuration parameters
     * 
     * In order to warn users about unused configuration file flags,
     * plugins should call call the SettingUsageRecorder::markAsUsed methods.
     *
     * The user is warned by the %theta main program about unused warnings based on the
     * recorded usage using this class.
     */
    class SettingUsageRecorder{
    public:
        /** \brief Mark a setting as used
         */
        void markAsUsed(const libconfig::Setting & s);
        
        /** \brief Get a list of paths of unused settings
         *
         * Searches recursively for unused settings.
         *
         * \param unused[out] The unused paths will be stored here
         * \param aggregate_setting[in] The setting to start the recursion; usually the root setting of the configuration file
         */
        void get_unused(std::vector<std::string> & unused, const libconfig::Setting & aggregate_setting) const;

    private:
        std::set<std::string> used_paths;
    };
    
    /** \brief A Setting in a configuration file
     *
     * It is assumed that you are familiar with the configuration file syntax and types. If not,
     * please read the libconfig documentation on these points first.
     *
     * SettingWrapper represents a single, read-only Setting in a configuration file. It overloads
     * casting operators to allow access to scalar types and index operators to access members
     * in aggregate type either by name (for setting groups) or by numeric index. Whenever
     * a value is read out, it will be recorded in an instance of SettingUsageRecorder. This
     * is mainly for user feedback of unused settings.
     *
     * For many methods, this class is merely a proxy to libconfig::Setting. Therefore, there will be many
     * references to the (very detailed) libconfig documentation in the documentation of this class.
     *
     * A setting is considered <it>used</it> one of the following methods are called on it:
     * <ul>
     *  <li>a casting operator</li>
     *  <li>access to a member in a compound with operator[]</li>
     *  <li>calling size()</li>
     * </ul>
     * The last definition is required to treat empty lists, groups and arrays correctly as used.
     */
    class SettingWrapper{
        const libconfig::Setting & rootsetting;
        mutable SettingUsageRecorder & rec;
        const libconfig::Setting & setting;
        
        static const libconfig::Setting & resolve_link(const libconfig::Setting & setting, const libconfig::Setting & root, SettingUsageRecorder & rec);
    public:
            //@{
            /** \brief Convert the current setting to the given type
             *
             * If the setting has not the correct ty[pe, a SettingTypeException will be thrown.
             */
            operator bool() const{
                rec.markAsUsed(setting);
                return setting;
            }
            
            operator std::string() const{
                rec.markAsUsed(setting);
                return setting;
            }
            
            operator int() const{
                rec.markAsUsed(setting);
                return setting;
            }
            
            operator unsigned int() const{
                rec.markAsUsed(setting);
                return setting;
            }            
            
            operator double() const{
                rec.markAsUsed(setting);
                return setting;
            }
            //@}
            
            /** \brief Get number of entries in a setting group, list or array
             *
             * Returns 0 if the setting is not a group, list or array. Same
             * as libconfig::Setting::getLength()
             */
            size_t size() const{
                rec.markAsUsed(setting);
                return setting.getLength();
            }
            
            /** \brief Get entry by index in a setting group, list or array
             *
             * same as libconfig::Setting::operator[](int)
             */
            SettingWrapper operator[](int i) const{
                rec.markAsUsed(setting);
                return SettingWrapper(setting[i], rootsetting, rec);
            }
            
            /** \brief Get entry by name in a setting group
             *
             * same as libconfig::Setting::operator[](string), with the only difference
             * that links of the form "@&lt;path&gt;" are followed
             */
            //@{
            SettingWrapper operator[](const std::string & name) const{
                rec.markAsUsed(setting);
                return SettingWrapper(setting[name], rootsetting, rec);
            }
            
            //This must be defined in addition to operator[](string). Otherwise,
            // an expression in SettingWrapper s; s["abc"]; could also be read as
            // (int)s ["abc"]  (which is the same as "abc"[(int)s]) and the compiler would not
            // have a clear priority ...
            SettingWrapper operator[](const char * name) const{
                return operator[](std::string(name));
            }
            //@}
            
            
           /** \brief Return a double, but allow the special strings "inf", "-inf" for infinity
            *
            * At some places in the configuration, it is allowed to use "inf" or "-inf" instead of a double.
            * This function is used to parse these settings.
            */
            double get_double_or_inf() const;
            
            /** \brief Returns whether a setting of this name exists in the current setting group
             *
             * same as libconfig::Setting::exists
             */
            bool exists(const std::string & path) const{
                return setting.exists(path);
            }
            
            /** \brief Returns the name of the current setting within its parent setting
             *
             * same as libconfig::Setting::getName
             */
            std::string getName() const{
                return setting.getName();
            }
            
            /** \brief Returns the configuration file path of the current setting
             *
             * same as libconfig::Setting::getPath
             */
            std::string getPath() const{
                return setting.getPath();
            }
            
            SettingWrapper(const libconfig::Setting & s, const libconfig::Setting & root, SettingUsageRecorder & recorder);
    };
}

#endif
