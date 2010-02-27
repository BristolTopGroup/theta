#include "interface/plugin_so_interface.hpp"
#include "interface/exception.hpp"
#include "Minuit2/Minuit2Minimizer.h"
#include "Math/IFunction.h"

#include "interface/plugin.hpp"
#include "interface/phys.hpp"
#include "interface/minimizer.hpp"

using namespace theta;
using namespace theta::plugin;
using namespace std;


// function adapter to be used by root minuit and
// "redirects" function calls to theta::Function
class RootMinuitFunctionAdapter: public ROOT::Math::IMultiGenFunction{
public:
    virtual ROOT::Math::IBaseFunctionMultiDim*	Clone() const{
        throw Exception("RootMinuitFunctionAdapter::Clone not implemented");
    }

    virtual unsigned int NDim() const{
        return ndim;
    }

    RootMinuitFunctionAdapter(const Function & f_): f(f_), ndim(f.getnpar()){
    }

    virtual double DoEval(const double * x) const{
        return f(x);
    }

private:
    const theta::Function & f;
    const size_t ndim;
};

class root_minuit: public Minimizer{
public:

    //method is either "simplex" or "migrad". Otherwise, an InvalidArgumentException
    // is thrown.
    root_minuit(Configuration & cfg);

    void set_printlevel(int p){
        min->SetPrintLevel(p);
    }
    
    //set to NAN to use default
    void set_tolerance(double tol){
        tolerance = tol;
    }

    virtual MinimizationResult minimize(const theta::Function & f){
        //I would like to re-use min. However, it horribly fails after very few uses with
        // unsigned int ROOT::Minuit2::MnUserTransformation::IntOfExt(unsigned int) const: Assertion `!fParameters[ext].IsFixed()' failed.
        // when calling SetFixedVariable(...).
        //Using a "new" one every time seems very wastefull, but it seems to work ...
        min.reset(new ROOT::Minuit2::Minuit2Minimizer(type));
        MinimizationResult result;

        //1. setup parameters, limits and initial step sizes
        ParIds parameters = f.getParameters();
        int ivar=0;
        for(ParIds::const_iterator it=parameters.begin(); it!=parameters.end(); ++it, ++ivar){
            pair<double, double> range = get_range(*it);
            double def = vm->get_default(*it);
            double step = get_initial_stepsize(*it);
            string name = vm->getName(*it);
            if(isinf(range.first)){
                if(isinf(range.second)){
                    min->SetVariable(ivar, name, def, step);
                }
                else{
                    min->SetUpperLimitedVariable(ivar, name, def, step, range.second);
                }
            }
            else{
                if(isinf(range.second)){
                    min->SetLowerLimitedVariable(ivar, name, def, step, range.first);
                }
                else{ // both ends are finite
                    if(range.first==range.second){
                        min->SetFixedVariable(ivar, name, range.first);
                    }
                    else{
                        min->SetLimitedVariable(ivar, name, def, step, range.first, range.second);
                    }
                }
            }
        }

        //2. setup the function
        RootMinuitFunctionAdapter minuit_f(f);
        min->SetFunction(minuit_f);

        //3. setup tolerance
        if(!isnan(tolerance))  min->SetTolerance(tolerance);
        
        //4. minimize
        bool success = min->Minimize();

        //5. do error handling
        if(not success){
            int status = min->Status();
            stringstream s;
            s << "MINUIT returned status " << status;
            switch(status){
                case 1: s << " (Covariance was made pos defined)"; break;
                case 2: s << " (Hesse is invalid)"; break;
                case 3: s << " (Edm is above max)"; break;
                case 4: s << " (Reached call limit)"; break;
                case 5: s << " (Some other failure)"; break;
                default:
                    s << " [unexpected status code]";
            }
            throw MinimizationException(s.str());
        }

        //6. convert result
        result.fval = min->MinValue();
        ivar = 0;
        const double * x = min->X();
        const double * errors = 0;
        bool have_errors = min->ProvidesError();
        if(have_errors) errors = min->Errors();
        for(ParIds::const_iterator it=parameters.begin(); it!=parameters.end(); ++it, ++ivar){
            result.values.set(*it, x[ivar]);
            if(have_errors){
                result.errors_plus.set(*it, errors[ivar]);
                result.errors_minus.set(*it, errors[ivar]);
            }
            else{
                result.errors_plus.set(*it, -1);
                result.errors_minus.set(*it, -1);
            }
        }
        result.covariance.reset(parameters.size(), parameters.size());
        //I would use min->CovMatrixStatus here to check the validity of the covariance matrix,
        // if only it was documented ...
        if(min->ProvidesError()){
            for(size_t i=0; i<parameters.size(); ++i){
                for(size_t j=0; j<parameters.size(); ++j){
                    result.covariance(i,j) = min->CovMatrix(i,j);
                }
            }
        }
        else{
            for(size_t i=0; i<parameters.size(); ++i){
                result.covariance(i,i) = -1;
            }
        }
        return result;
    }

private:
    ROOT::Minuit2::EMinimizerType type;
    std::auto_ptr<ROOT::Minuit2::Minuit2Minimizer> min;
    double tolerance;
};

/** \brief Factory for the ROOT Minuit Minimizer.
 *
 * Configuration with a setting like:
 * <pre>
 * {
 * type = "root-minuit";
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
root_minuit::root_minuit(Configuration & ctx): Minimizer(ctx.vm), tolerance(NAN){
       min.reset(new ROOT::Minuit2::Minuit2Minimizer(type));
       int printlevel = 0;
       if(ctx.setting.lookupValue("printlevel", printlevel)){
           ctx.rec.markAsUsed(ctx.setting["printlevel"]);
       }
       string method = "migrad";
       if(ctx.setting.lookupValue("method", method)){
           ctx.rec.markAsUsed(ctx.setting["method"]);
       }
       if(method=="migrad"){
            type = ROOT::Minuit2::kMigrad;
       }
       else if(method == "simplex"){
           type = ROOT::Minuit2::kSimplex;
       }
       else{
           stringstream s;
           s << "RootMinuit: invalid method '" << method << "' (allowed are only 'migrad' and 'simplex')";
           throw InvalidArgumentException(s.str());
       }       
       double tol = NAN;
       if(ctx.setting.lookupValue("tolerance", tol)){
           ctx.rec.markAsUsed(ctx.setting["tolerance"]);
       }
       set_printlevel(printlevel);
       set_tolerance(tol);
       MinimizerUtils::apply_settings(*this, ctx);
   }


REGISTER_PLUGIN(root_minuit)
