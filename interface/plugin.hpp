#ifndef PLUGIN_HPP
#define PLUGIN_HPP

#include "interface/decls.hpp"
#include "interface/exception.hpp"
#include "interface/cfg-utils.hpp"

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

#include <sstream>
#include <iostream>
#include <dlfcn.h>

namespace theta {

    /** \brief Namespace for all plugin-related classes.
     */
    namespace plugin {
        
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
             * If a "name" setting exists, it will be used as name. Otherwise,
             * the name of the configuration setting group is used.
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
            PluginType(const Configuration & c);
            
            /** \brief Provide default constructor for derived classes
             *
             * In order to allow construction outside of the plugin system (i.e., without
             * a Configuration instance), this default constructor is provided for derived classes.
             */
            PluginType(){}

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
            void set_table(const boost::shared_ptr<theta::EventTable> & table_){
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
            EventTableWriter(const std::string & name);
            
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
            
            /** \brief Construct Configuration by specifying all data members
             */
            Configuration(const boost::shared_ptr<VarIdManager> & vm_, const SettingWrapper & setting_): vm(vm_),
                setting(setting_){}

            /** \brief Copy elements from another Configuration, but replace Configuration::setting
             *
             * Copy all from \c cfg but \c cfg.setting which is replaced by \c setting_.
             */
            Configuration(const Configuration & cfg, const SettingWrapper & setting_): vm(cfg.vm),
                setting(setting_){}

        };        
        
        
        template<typename> class PluginManager;
        /** \brief Class used internally for the PluginManager
         *
         * You usually do not have to care about it, as this is a detail handeled by the REGISTER_PLUGIN macro.
         *
         * This is the abstract factory class for a certain \c base_type. For each \c base_type, there
         * is an instance of PluginManager&lt;base_type&gt; which will save pointers to all currently registered
         * factories of type factory&lt;base_type&gt;
         *
         * By use of the REGISTER_PLUGIN(some_concrete_type), a derived class of factory&lt;some_concrete_type::base_type&gt;
         * will be created which handles the construction of \c some_concrete_type. This factory will be registered
         * at the PluginManager&lt;some_conrete_type::base_type&gt; as soon as the shared object file is loaded.
         */
         template<typename base_type>
         class factory{
         public:
             /// build an instance from a Configuration object
             virtual std::auto_ptr<base_type> build(const Configuration & cfg) = 0;
             
             /// the type of the object this factory is responsible for; it corresponds to the type="..." configuration file setting
             virtual std::string get_typename() = 0;
         protected:
             /// register this factory at the correct PluginManager
             void reg(){
                 PluginManager<base_type>::register_factory(this);
             }
         };

         //helper macros for REGISTER_PLUGIN
         #define CONCAT(a,b) CONCAT2(a,b)
         #define CONCAT2(a,b) a ## b

         /* define a template specialization of abstract_factory<base_type> which constructs the desired type
         */
         #define REGISTER_PLUGIN_NAME(type,name) namespace { class CONCAT(factory,__LINE__): public theta::plugin::factory<type::base_type>{ \
         public:\
         virtual std::auto_ptr<type::base_type> build(const theta::plugin::Configuration & cfg){return auto_ptr<type::base_type>(new type(cfg)); }\
         virtual std::string get_typename(){ return #name ;}\
         CONCAT(factory,__LINE__)(){reg();}\
         }; CONCAT(factory,__LINE__) CONCAT(factory_instance,__LINE__);}
         
         #define REGISTER_PLUGIN(type) REGISTER_PLUGIN_NAME(type, type)

        /** \brief Central registry class for plugins.
         *
         * This class serves two purposes:
         * <ol>
         * <li>To build an instance of some class from a Configuration</li>
         * <li>To serve as central registry for all plugins of a certain type to fulfill 1.</li>
         * </ol>
         * Registration in 2. is usually done through the REGISTER_PLUGIN macro, so the user
         * usually does not have to care about the details of plugin registration.
         *
         * Note that this class is <b>not</b> thread-safe, as it has access to a global data (the plugin
         * registry is global). If used in a multi-threaded program, the user has to ensure that no two threads
         * call any method of this class at the same time.
         */
        template<typename product_type>
        class PluginManager : private boost::noncopyable {
        public:

            /** \brief Use the registered factories to build an instance from a configuration settings block.
             *
             * This will go through all registered factories of that C++-type and
             * find the factory responsible by using the "type=..." setting in ctx.setting.
             */
            static std::auto_ptr<product_type> build(const Configuration & cfg);

            /** \brief get all currently registered types.
             */
            static std::vector<std::string> get_registered_types();
        private:
            typedef typename product_type::base_type base_type;
            typedef factory<base_type> factory_type;
            friend class factory<base_type>;
            
            /** \brief Register a new factory.
             *
             * An InvalidArgumentException is thrown if there already is
             * a registered plugin for the same type string.
             *
             * Used by the REGISTER_PLUGIN macro. Not for direct call.
             */
            static void register_factory(factory_type * new_factory);
            static std::vector<factory_type*> factories;
            PluginManager() {}
        };
        
        template<typename product_type>
        std::vector<typename PluginManager<product_type>::factory_type*> PluginManager<product_type>::factories;

        template<typename product_type>
        std::vector<std::string> PluginManager<product_type>::get_registered_types() {
            std::vector<std::string> names;
            names.reserve(factories.size());
            for (size_t i = 0; i < factories.size(); i++) {
                names.push_back(factories[i]->get_typename());
            }
            return names;
        }

        template<typename product_type>
        std::auto_ptr<product_type> PluginManager<product_type>::build(const Configuration & ctx){
            std::string type = "<unspecified>";
            for (size_t i = 0; i < factories.size(); ++i) {
                try {
                    //as the access to setting["type"] might throw, also write it into the
                    // try block ...
                    type = std::string(ctx.setting["type"]);
                    if (factories[i]->get_typename() == type) {
                        return factories[i]->build(ctx);
                    }
                }catch (Exception & ex) {
                    std::stringstream ss;
                    ss << "PluginManager<" << typeid(product_type).name() << ">::build, configuration path '" << ctx.setting.getPath()
                       << "', type='" << type << "' error while building from plugin: " << ex.message;
                    ex.message = ss.str();
                    throw;
                }
            }
            std::stringstream ss;
            ss << "PluginManager::build<" << typeid(product_type).name() << ">, configuration path '" << ctx.setting.getPath()
               << "': no plugin found to create type='" << type << "'";
            throw ConfigurationException(ss.str());
        }

        template<typename product_type>
        void PluginManager<product_type>::register_factory(factory_type * new_factory) {
            for (size_t i = 0; i < factories.size(); i++) {
                if (factories[i]->get_typename() == new_factory->get_typename()) {
                    std::stringstream ss;
                    ss << "PluginManager::register_factory: there is already a plugin registered for type '" << factories[i]->get_typename() << "'";
                    throw InvalidArgumentException(ss.str());
                }
            }
            //append plugins to the end of the list:
            factories.push_back(new_factory);
        }

        /** \brief Class responsible to load the shared object files containing plugins
         */
        class PluginLoader {
        public:

            /** \brief Run the loader according to the setting cfg.setting
             *
             * cfg.sertting must contain the "filenames" setting, a list of strings with paths of .so files.
             * Optionally, it may contain a boolean "verbose" which controls the verbosity level of
             * the plugin loader.
             */
            static void execute(const theta::plugin::Configuration & cfg);

            /** \brief print a list of all currently available plugins to standard out, grouped by type
             *
             * This is mainly for debugging.
             */
            static void print_plugins();

            /** \brief load a plugin file
             *
             * The given shared object file will be loaded which will trigger the plugin registration of all plugins
             * defined via the REGISTER_PLUGIN macro automagically.
             *
             * \param soname is the filename of the plugin (a .so file), including the path from the current directory
             */
            static void load(const std::string & soname);
        };
    }
}

#endif
