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

        /** \brief Abstract class for all plugins writing to the "products" table
         *
         * Any plugin which writes to the "products" table must derive from this base class.
         * Currently, the DataSource and the Producer derive from this class.
         */
        class ProductsTableWriter{
            
        public:
        
            /** \brief Set the table where the results should be written to
            *
            * This is called during the setup phase of a theta::Run. It is called just before
            * define_table, which does the producer-specific table definition.
            */
            void set_table(const boost::shared_ptr<theta::ProductsTable> & table_){
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
            
            /// Declare Destrutor virtual as we will probably have polymorphic access to derived classes
            virtual ~ProductsTableWriter();
            
            /// Returns the name of this instance, as set in the configuration file via name="...";
            std::string getName() const{
                return name;
            }
            
            /// Returns the type of this instance, as given in the configuration file via type="...";
            std::string getType() const{
                return type;
            }
        
        protected:
            ProductsTableWriter(const Configuration & cfg);
            
            /// The table instance to be used for writing    
            boost::shared_ptr<ProductsTable> table;
            
        private:
                std::string type, name;
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
         virtual std::auto_ptr<type::base_type> build(const theta::plugin::Configuration & cfg){return std::auto_ptr<type::base_type>(new type(cfg)); }\
         virtual std::string get_typename(){ return #name ;}\
         CONCAT(factory,__LINE__)(){reg();}\
         }; CONCAT(factory,__LINE__) CONCAT(factory_instance,__LINE__);}
         
         #define REGISTER_PLUGIN(type) REGISTER_PLUGIN_NAME(type, type)
         #define REGISTER_PLUGIN_DEFAULT(type) REGISTER_PLUGIN_NAME(type, default)
         
         //we need to make explicit template instantiations of the PluginManager registry,
         // otherwise, the central registry associated to the PluginManager does not work reliably.
         // This can happen if theta core does not instantiate the PluginManager class itself but some plugin does.
         // The exact reason however is unknown ...
         #define REGISTER_PLUGIN_BASETYPE(type) template class theta::plugin::PluginManager<type>

        // to prevent endless recursion within PluginManager::build, use a depth counter:
        extern int plugin_build_depth;

       //to increase the plugin_build_depth in an exception-safe manner, use the build_depth_sentinel,
       // which automatically decreses depth count at destrution:
       struct plugin_build_depth_sentinel{
           plugin_build_depth_sentinel(){ ++plugin_build_depth; }
           ~plugin_build_depth_sentinel(){  --plugin_build_depth; }
       };

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
        class PluginManager: private boost::noncopyable {
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
            //prevent instance construction by making constructor private:
            PluginManager(){}
            
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
            plugin_build_depth_sentinel b;
            if(plugin_build_depth > 10){
                throw FatalException("PluginManager::build: detected too deep plugin building");
            }
            std::string type;
            if(!ctx.setting.exists("type")) type = "default";
            else type = static_cast<std::string>(ctx.setting["type"]);
            for (size_t i = 0; i < factories.size(); ++i) {
                if (factories[i]->get_typename() != type) continue;
                try {
                    return factories[i]->build(ctx);
                }catch (Exception & ex) {
                    std::stringstream ss;
                    ss << "PluginManager<" << typeid(product_type).name() << ">::build, configuration path '" << ctx.setting.getPath()
                       << "', type='" << type << "': " << ex.message;
                    ex.message = ss.str();
                    throw;
                }
            }
            std::stringstream ss;
            ss << "PluginManager<" << typeid(product_type).name() << ">::build, configuration path '" << ctx.setting.getPath()
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
            factories.push_back(new_factory);
        }

        /** \brief Class responsible to load the shared object files containing plugins
         */
        class PluginLoader {
        public:

            /** \brief Run the loader according to the configuration file setting
             *
             * The shared-object files in the \c plugins_files setting are loaded. This
             * setting must be a list of strings of the filenames of the .so files.
             */
            static void execute(const theta::plugin::Configuration & cfg);

            /** \brief load a single plugin file
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
