#ifndef PLUGIN_SO_INTERFACE_HPP
#define PLUGIN_SO_INTERFACE_HPP

#include "interface/decls.hpp"
#include "interface/cfg-utils.hpp"
#include <boost/shared_ptr.hpp>

namespace theta{
    namespace plugin{
        
        /** \brief The abstract base class for all plugin types
         *
         * It is common to all classes which can be implemented as plugin to have
         * (through the configuration) a "name" and a "type". This common aspect is
         * implemented with this class.
         */
        class PluginType{
        public:

            /** \brief Returns the name of the instance, as given in the configuration file
             *
             * The name of the configuration setting group used to construct this instance will be
             * saved and returned here.
             */
            std::string get_name() const{
                return name;
            }
            
            /** \brief Returns the type, as set in the configuration file, for this instance
             *
             */
            std::string get_type() const{
                return type;
            }
             
        protected:
            /** \brief Construct, filling name and type from the supplied Configuration
             */
            PluginType(const Configuration & c){
                name = c.setting.getName();
                type = c.setting["type"];
            }
            
            /** \brief Provide default constructor for derived classes
             *
             * In order to allow construction outside of the plugin system (i.e., without
             * a Configuration instance), this default constructor is provided for derived classes.
             */
            //PluginType(){}

        private:
            std::string name;
            std::string type;
        };
        
        /** \brief Abstract base class for all PluginTypes writing to an EventTable
         *
         * PluginTypes making use of the EventTable class should derive from this class.
         */
        class EventTableWriter{
            
        public:
        
            /** \brief Set the table where the results should be written to
            *
            * This is called during the setup phase of a theta::Run. It is called just before
            * define_table, which does the producer-specific table definition.
            */
            void set_table(const boost::shared_ptr<theta::ProducerTable> & table_){
                table = table_;
            }
        
            /** \brief Define the columns of the result table
            *
            * This method is implemented by derived classes. It should call table->add_column for
            * each column it wants to fill in the produce method.
            *
            * Implementations may assume that the table pointer is valid upon invocation of this method.
            */
            virtual void define_table() = 0;
        
        protected:
            /// The table instance to be used for writing    
            boost::shared_ptr<EventTable> table;
        };

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
