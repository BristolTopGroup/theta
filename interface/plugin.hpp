#ifndef PLUGIN_HPP
#define	PLUGIN_HPP

#include "interface/plugin_so_interface.hpp"
#include "interface/exception.hpp"
#include "interface/cfg-utils.hpp"
#include "libconfig/libconfig.h++"

#include <sstream>
#include <iostream>
#include <dlfcn.h>

#include <boost/thread.hpp>


namespace theta {

    /** \brief Namespace for all plugin-related classes.
     */
    namespace plugin {

        /** For a given class, there can be many plugins which provide each one or more factories
         *  which can build an instance of such a class from a \c ConfigurationContext. The \c PluginManager &lt; T &gt;
         * has the task of managing all T-Factories: it serves as a central register of factories and delegates
         * the construction to the corect factory.
         *
         * For any T, \c PluginManager &lt; T &gt; is a singleton.
         */
        template<typename factory_type>
        class PluginManager : private boost::noncopyable {
        public:
            typedef boost::ptr_vector<factory_type> plugin_array;
            typedef typename factory_type::product_type product_type;

            /** \brief returns the singleton instance of this PluginManager.
             */
            static PluginManager* get_instance() {
                boost::lock_guard<boost::mutex> l(singleton_mutex);
                if (instance.get() == 0) instance.reset(new PluginManager);
                return instance.get();
                //singleton_mutex is released implicitely upon destruction of l
            }

            /** \brief Use the registered factories to build an instance from a configuration
             *   settings block.
             *
             * This will go through all registered factories of that C++-type and
             * find the factory responsible by using the "type=..." setting.
             */
            std::auto_ptr<product_type> build(ConfigurationContext & ctx) const;

            /** \brief Register a new factory.
             *
             * The ownership of ptr is transferred to this class: new_factory.get() will be 0
             * if this function returns successfully.
             *
             * An InvalidArgumentException is thrown if there already is
             * a registered plugin for the same type string.
             */
            void register_factory(std::auto_ptr<factory_type> & new_factory);

            /** \brief get all currently registered types.
             */
            std::vector<std::string> get_registered_types();
        private:
            //bool verbose;
            boost::ptr_vector<factory_type> factories;
            static std::auto_ptr<PluginManager> instance;
            //mutex to prevent more than one function executing in parallel
            // (and to prevent more than one instance being created).
            static boost::mutex singleton_mutex;
            //prevent public default construction.

            PluginManager() {
            }
        };

        template<typename factory_type>
        std::auto_ptr<PluginManager<factory_type> > PluginManager<factory_type>::instance;

        template<typename factory_type>
        boost::mutex PluginManager<factory_type>::singleton_mutex;

        template<typename factory_type>
        std::vector<std::string> PluginManager<factory_type>::get_registered_types() {
            std::vector<std::string> names;
            names.reserve(factories.size());
            for (size_t i = 0; i < factories.size(); i++) {
                names.push_back(factories[i].getTypeName());
            }
            return names;
        }

        template<typename factory_type>
        std::auto_ptr<typename factory_type::product_type> PluginManager<factory_type>::build(ConfigurationContext & ctx) const {
            boost::lock_guard<boost::mutex> l(singleton_mutex);
            std::string type = ctx.setting["type"];
            ctx.rec.markAsUsed(ctx.setting["type"]);
            for (size_t i = 0; i < factories.size(); ++i) {
                if (factories[i].getTypeName() == type) {
                    try {
                        return factories[i].build(ctx);
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

        template<typename factory_type>
        void PluginManager<factory_type>::register_factory(std::auto_ptr<factory_type> & new_factory) {
            boost::lock_guard<boost::mutex> l(singleton_mutex);
            for (size_t i = 0; i < factories.size(); i++) {
                if (factories[i].getTypeName() == new_factory->getTypeName()) {
                    std::stringstream ss;
                    ss << "PluginManager::register_factory: there is already a plugin registered for type '" << factories[i].getTypeName() << "'";
                    throw InvalidArgumentException(ss.str());
                }
            }
            //append plugins to the end of the list:
            factories.push_back(new_factory);
            assert(new_factory.get() == 0);
            //release singleton_mutex upon destrution of l.
        }

        /** \brief Class responsible to register the plugins at the different PluginManagers.
         *
         * This is the class which ties it all together: It knows
         * <ul>
         * <li>which PluginManagers there are in the whole program </li>
         * <li>where and how to load the shared library providing plugins for
         *   the different plugin types.</li>
         * <li>the (hard-coded) plugin-interface to get the factories from the plugin (.so files).</li>
         * </ul>
         */
        class PluginLoader {
        public:

            /** \brief Run the loader according to the plugins Setting \c s.
             */
            static void execute(const libconfig::Setting & s, theta::utils::SettingUsageRecorder & rec);

            /** \brief print a list of all currently available plugins to cout.
             *
             * This is mainly for debugging.
             */
            static void print_plugins();

            /** \brief load a plugin file.
             *
             * soname is the filename of the plugin (a .so file). The .so file
             * will be loaded which will trigger the plugin registration via the
             * REGISTER_FACTORY macro automagically.
             */
            static void load(const std::string & soname);
        };

    }
}//namespace theta::plugin

#endif	/* _HISTO_SOURCE_HPP */
