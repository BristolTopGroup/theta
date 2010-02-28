#ifndef PLUGIN_HPP
#define PLUGIN_HPP

#include "interface/plugin_so_interface.hpp"
#include "interface/exception.hpp"
#include "interface/cfg-utils.hpp"
#include "libconfig/libconfig.h++"

#include <boost/utility.hpp>

#include <sstream>
#include <iostream>
#include <dlfcn.h>

namespace theta {

    /** \brief Namespace for all plugin-related classes.
     */
    namespace plugin {
        template<typename> class PluginManager;
        /** \brief Class used internally for the PluginManager
         *
         * You usually do not have to care about it, as this is a detail handeled by the REGISTER_PLUGIN macro.
         *
         * This is the abstract factory class for a certain \c base_type. For each \c base_type, there
         * an instance of PluginManager&lt;base_type&gt; will save pointers to all currently registered
         * factories.
         *
         * By use of the REGISTER_PLUGIN(some_concrete_type), a derived class of factory&lt;some_concrete_type::base_type&gt;
         * will be created which handles the construction of \c some_concrete_type. This factory will be registered
         * at the PluginManager&lt;some_conrete_type::base_type&gt; as soon as the shared object file is loaded.
         */
         template<typename base_type>
         class factory{
         public:
             virtual std::auto_ptr<base_type> build(Configuration & cfg) = 0;
             virtual std::string get_typename() = 0;
         protected:
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
         virtual std::auto_ptr<type::base_type> build(theta::plugin::Configuration & cfg){return auto_ptr<type::base_type>(new type(cfg)); }\
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
            static std::auto_ptr<product_type> build(Configuration & ctx);

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
        std::auto_ptr<product_type> PluginManager<product_type>::build(Configuration & ctx){
            std::string type = ctx.setting["type"];
            ctx.rec.markAsUsed(ctx.setting["type"]);
            for (size_t i = 0; i < factories.size(); ++i) {
                if (factories[i]->get_typename() == type) {
                    try {
                        return factories[i]->build(ctx);
                    } catch (Exception & ex) {
                        std::stringstream ss;
                        ss << "PluginManager::build, configuration path '" << ctx.setting.getPath()
                                << "', type='" << type << "' error while building from plugin: " << ex.message;
                        ex.message = ss.str();
                        throw;
                    }
                }
            }
            std::stringstream ss;
            ss << "PluginManager::build, configuration path '" << ctx.setting.getPath()
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

            /** \brief Run the loader according to the plugins Setting \c s
             *
             * s must contain the "filenames" setting, a list of strings with paths of .so files.
             */
            static void execute(const libconfig::Setting & s, theta::utils::SettingUsageRecorder & rec);

            /** \brief print a list of all currently available plugins to standard out, grouped by type
             *
             * This is mainly for debugging.
             */
            static void print_plugins();

            /** \brief load a plugin file
             *
             * The given shared object file will be loaded which will trigger the plugin registration of all plugins
             * defined via the REGISTER_PLUGIN macro automagically. If the method is called more than once for the same filename,
             * all but the first call are ignored.
             *
             * \param soname is the filename of the plugin (a .so file), including the path from the current directory
             */
            static void load(const std::string & soname);
            
        private:
            static std::vector<std::string> loaded_files;
        };
    }
}

#endif
