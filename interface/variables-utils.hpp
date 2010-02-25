#ifndef VARIABLES_UTILS_HPP
#define VARIABLES_UTILS_HPP

#include "interface/plugin_so_interface.hpp"

namespace theta{
   namespace VarIdManagerUtils{
   
      /** \brief Populate VarIdManager from a Setting.
       *
       * Uses the "observables" and "parameters" settings to fill the VarIdManager instance in \c ctx.
       *
       * \todo document more here (setting parsed)
       */
      void apply_settings(theta::plugin::Configuration & ctx);
   }
}

#endif

