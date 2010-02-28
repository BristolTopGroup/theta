#ifndef CFG_UTILS_HPP
#define	CFG_UTILS_HPP

#include "libconfig/libconfig.h++"
#include "interface/exception.hpp"

#include <string>
#include <set>
#include <vector>
#include <sstream>


namespace theta{ namespace utils{

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
        //@{
        /** \brief Mark a setting as used
         *
         * The setting can either be identified by path or be used directly.
         */
        void markAsUsed(const std::string & path){
           used_paths.insert(path); 
        }
        void markAsUsed(const libconfig::Setting & s){
           used_paths.insert(s.getPath());
        }
        //@}
        /** \brief removes any unused setting from the given setting
         *
         * \param rootsetting is the root setting of the file.
         */
        void remove_used(libconfig::Setting & rootsetting) const{
            for(std::set<std::string>::const_iterator it = used_paths.begin(); it!=used_paths.end(); ++it){
                if(rootsetting.exists(*it)) rootsetting.remove(*it);
            }
        }
    private:
        std::set<std::string> used_paths;
    };
    

    /** \brief Recursively remove unused setting groups
     */
    void remove_empty_groups(libconfig::Setting & s);
   
   /** \brief Return a list of all paths in a setting
    *
    * Used to report unused settings
    */
   std::vector<std::string> get_paths(libconfig::Setting & s);
   
   /** \brief Parse the setting to return either a double value or plus/minus infinity
    *
    * At some places in the configuration, it is allowed to use "inf" or "-inf" instead of a double.
    * This function is used to parse these settings.
    */
   double get_double_or_inf(libconfig::Setting & s);
   
}}

#endif	/* _CFG_PARSER_HPP */
