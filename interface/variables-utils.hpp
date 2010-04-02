#ifndef VARIABLES_UTILS_HPP
#define VARIABLES_UTILS_HPP

#include "interface/decls.hpp"

namespace theta{
   namespace VarIdManagerUtils{
   
      /** \brief Populate VarIdManager from a Setting.
       *
       * This function uses the "observables" and "parameters" setting groups in cfg.setting to
       * populate cfg.VarIdManager.
       *
       * If \c cfg.settings is this settings group:
       * <pre>
       * {
       *  observables = {
       *     mass = {
       *        nbins = 200;
       *        range = [0.0, 10.0];
       *    };
       *  };
       *
       *  parameters = {
       *     p0 = {
       *         default = 20.0;
       *         range = (0.0, "inf");
       *   };
       *  };
       * }
       * </pre>
       * then this function will call VarIdManager::createParId("p0", 20, 0, inf) and VarIdManager::createObsId("mass", 200, 0.0, 10.0).
       */
      void apply_settings(theta::plugin::Configuration & cfg);
   }

   std::ostream & operator<<(std::ostream & out, const ParIds & pids);
}

#endif

