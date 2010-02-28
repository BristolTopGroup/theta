#ifndef THETA_ROOT_MINUIT_HPP
#define THETA_ROOT_MINUIT_HPP

#include "interface/plugin_so_interface.hpp"
#include "interface/exception.hpp"
#include "Minuit2/Minuit2Minimizer.h"
#include "Math/IFunction.h"

#include "interface/plugin.hpp"
#include "interface/phys.hpp"
#include "interface/minimizer.hpp"

/** \brief Minimizer using the MINUIT minimizer from root
 *
 * Configuration with a setting like:
 * <pre>
 * {
 * type = "root_minuit";
 * //general minimization parameters:
 * override-ranges = {
 *     some_param_name = (0.0, 2.0);
 * };
 * initial-step-sizes = {
 *     some_param_name = 0.1;
 * };
 *
 *  //root-minuit specific:
 *  printlevel = 1; //optional. Used in call to TMinuit::SetPrintLevel(). Default is 0/
 *  method = "simplex"; //optional. Default is "migrad".
 *  tolerance = 0.001; //optional. 
 * }
 * </pre>
 *
 * \c method must be either "simplex" or "migrad".
 *
 * \c tolerance is the Tolerance as should be documented in ROOT::Minuit2::Minuit2Minimizer::SetTolerance.
 *  Default is the one used by ROOT::Minuit2::Minuit2Minimizer and which should be documented by ROOT (if
 *  you find this documentation, write me a mail, so I can point other users to it here).
 *
 */
class root_minuit: public theta::Minimizer{
public:
    /** \brief Constructor used by the plugin system to build an instance from a configuration file.
     */
    root_minuit(Configuration & cfg);

    /** \brief Implement the Minimizer::minimize routine.
     *
     * See documentation of Minimizer::minimize for an introduction
     *
     * The minimizer forwards the task to a ROOT::Minuit2::Minuit2Minimizer using its
     * functions also for setting limits on the parameters. If minimization fails, it
     * is attempted up to three times through repeating calls of ROOT::Minuit2::Minuit2Minimizer::Minimize.
     * If still an error is reported, a MinimizerException is thrown which contains the status
     * code returned by ROOT::Minuit2::Minuit2Minimizer::Status().
     */
    virtual MinimizationResult minimize(const theta::Function & f);
private:
    void set_printlevel(int p);
    
    //set to NAN to use default
    void set_tolerance(double tol);
    
    ROOT::Minuit2::EMinimizerType type;
    std::auto_ptr<ROOT::Minuit2::Minuit2Minimizer> min;
    double tolerance;
};

#endif
