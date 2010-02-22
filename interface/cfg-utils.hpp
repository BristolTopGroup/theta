#ifndef CFG_UTILS_HPP
#define	CFG_UTILS_HPP

#include "libconfig/libconfig.h++"
#include "interface/exception.hpp"

#include <string>
#include <set>
#include <vector>
#include <sstream>


namespace theta{ namespace utils{

    /* \brief Keep record of the usage of configuration parameters.
     * 
     * In order to warn users about unused configuration file flags,
     * factory functions keep track of used  
     * 
     * 
     */
    class SettingUsageRecorder{
    public:
        void markAsUsed(const std::string & path){
           used_paths.insert(path); 
        }
        void markAsUsed(const libconfig::Setting & s){
           used_paths.insert(s.getPath());
        }
        void remove_used(libconfig::Setting & rootsetting) const{
            for(std::set<std::string>::const_iterator it = used_paths.begin(); it!=used_paths.end(); ++it){
                if(rootsetting.exists(*it)) rootsetting.remove(*it);
            }
        }
    private:
        std::set<std::string> used_paths;
    };
    

   void remove_empty_groups(libconfig::Setting & s);
   std::vector<std::string> get_paths(libconfig::Setting & s);
   double get_double_or_inf(libconfig::Setting & s);
   
   /* Catched exceptions typically arising while using the factory functions.
    */
   /*template<class R, class T>
   R ExceptionWrapper(T & t){
       using namespace libconfig;
       try{
           return t();
       } catch (SettingNotFoundException & ex) {
        std::stringstream s;
        s << "The required configuration parameter " << ex.getPath() << " was not found.";
        throw ConfigurationException(s.str());
    } catch (SettingTypeException & ex) {
        std::stringstream s;
        s << "The configuration parameter " << ex.getPath() << " has the wrong type.";
        throw ConfigurationException(s.str());
    } catch (SettingException & ex) {
        std::stringstream s;
        s  << "An unspecified setting exception while processing the configuration occured in path "
                << ex.getPath();
        throw ConfigurationException(s.str());
    } catch (Exception & e) {
        std::stringstream s;
        s << "An exception occured while processing the configuration: " << e.message;
        throw ConfigurationException(s.str());
    }
   }*/
   
}}

#endif	/* _CFG_PARSER_HPP */
