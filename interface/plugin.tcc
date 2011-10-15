#ifndef PLUGIN_TCC
#define PLUGIN_TCC

// This is the file implementing the class templates from plugin.hpp.
// It needs to be included only by those .cpp files which want to make a template
// instantiation via REGISTER_PLUGIN_BASETYPE, while most other only need to include
// plugin.hpp (including those calling REGISTER_PLUGIN).
// Splitting this section from the header file saves some compile time and space.

#include "interface/plugin.hpp"
#include "interface/exception.hpp"

//we need to make explicit template instantiations of the PluginManager registry,
// otherwise, the central registry associated to the PluginManager does not work reliably.
// This can happen if theta core does not instantiate the PluginManager class itself but some plugin does.
// The exact reason however is unknown ...
#define REGISTER_PLUGIN_BASETYPE(type) template class theta::PluginManager<type>;

using namespace theta;

namespace{
    //to increase the plugin_build_depth in an exception-safe manner, use the build_depth_sentinel,
    // which automatically decreaes depth count at destruction:
    struct plugin_build_depth_sentinel{
        int & i;
        plugin_build_depth_sentinel(int & i_):i(i_){ ++i; }
        ~plugin_build_depth_sentinel(){ --i; }
    };
}

template<typename product_type>
PluginManager<product_type> & PluginManager<product_type>::instance(){
    static PluginManager pm;
    return pm;
}

template<typename product_type>
PluginManager<product_type>::PluginManager(): plugin_build_depth(0) {}

template<typename product_type>
std::auto_ptr<product_type> PluginManager<product_type>::build(const Configuration & ctx, const std::string & type_){
    PluginManager & pm = instance();
    plugin_build_depth_sentinel b(pm.plugin_build_depth);
    if(pm.plugin_build_depth > 15){
        throw FatalException("PluginManager::build: detected too deep plugin building");
    }
    std::string type = type_;
    if(type==""){
        if(!ctx.setting.exists("type")) type = "default";
        else type = static_cast<std::string>(ctx.setting["type"]);
        if(type=="") throw ConfigurationException("Error while constructing plugin: empty 'type' setting given in path '" + ctx.setting.getPath() + "'");
        if(pm.pb.get()){
            return pm.pb->build(ctx, type);
        }
    }
    for (size_t i = 0; i < pm.factories.size(); ++i) {
        if (pm.factories[i]->get_typename() != type) continue;
        try {
            return pm.factories[i]->build(ctx);
        }catch (Exception & ex) {
            std::stringstream ss;
            ss << "Error while constructing plugin according to configuration path '" << ctx.setting.getPath()
                << "' (type='" << type << "'): " << ex.message;
            ex.message = ss.str();
            throw;
        }
    }
    std::stringstream ss;
    ss << "Error while constructing plugin according to configuration path '" << ctx.setting.getPath()
        << "': no plugin registered to create type='" << type << "'. Check spelling of the "
        "type and make sure to load all necessary plugin files via the setting 'options.plugin_files'";
    throw ConfigurationException(ss.str());
}

template<typename product_type>    
void PluginManager<product_type>::set_plugin_builder(std::auto_ptr<PluginBuilder<product_type> > & b){
    PluginManager & pm = instance();
    pm.pb = b;
}
    

template<typename product_type>
void PluginManager<product_type>::reset_plugin_builder(){
    PluginManager & pm = instance();
    pm.pb.reset();
}

template<typename product_type>
void PluginManager<product_type>::register_factory(factory_type * new_factory) {
    PluginManager & pm = instance();
    for (size_t i = 0; i < pm.factories.size(); i++) {
        if (pm.factories[i]->get_typename() == new_factory->get_typename()) {
            std::stringstream ss;
            ss << "PluginManager::register_factory: there is already a plugin registered for type '" << pm.factories[i]->get_typename() << "'";
            throw InvalidArgumentException(ss.str());
        }
    }
    pm.factories.push_back(new_factory);
}


#endif
