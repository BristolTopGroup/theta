#include "root/root_minuit.hpp"

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

void root_minuit::set_printlevel(int p){
    min->SetPrintLevel(p);
}

//set to NAN to use default
void root_minuit::set_tolerance(double tol){
    tolerance = tol;
}

MinimizationResult root_minuit::minimize(const theta::Function & f){
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
        double def = get_default(*it);
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
    //3.a. error definition. Unfortunately, SetErrorDef in ROOT is not documented, so I had to guess.
    // 0.5 seems to work somehow.
    min->SetErrorDef(0.5);
    
    //4. minimize. In case of failure, try harder
    bool success;
    for(int i=1; i<=3; i++){
        success = min->Minimize();
        if(success) break;
    }
    //4.a. get minos errors:
    /*ParValues minos_errors_plus;
    ParValues minos_errors_minus;
    if(success){
        for(ParIds::const_iterator it=minos_parids.begin(); it!=minos_parids.end(); ++it){
            //find the parameter index of *it:
            ParIds::const_iterator pos_in_par = parameters.begin();
            ivar = 0;
            while(pos_in_par != parameters.end() && *pos_in_par!=*it){
                ++pos_in_par;
                ++ivar;
            }
            //if parameter *it is not in the list of current parameters, skip:
            if(pos_in_par == parameters.end()) continue;
            
            double minos_plus;
            double minos_minus;
            if(not min->GetMinosError(ivar, minos_minus, minos_plus)){
                success = false;
                break;
            }
            minos_errors_plus.set(*it, minos_plus);
            minos_errors_minus.set(*it, minos_minus);
        }
    }*/

    //5. do error handling
    if(not success){
        int status = min->Status();
        int status_1 = status % 10;
        //int status_2 = status / 10;
        stringstream s;
        s << "MINUIT returned status " << status;
        switch(status_1){
            case 1: s << " (Covariance was made pos defined)"; break;
            case 2: s << " (Hesse is invalid)"; break;
            case 3: s << " (Edm is above max)"; break;
            case 4: s << " (Reached call limit)"; break;
            case 5: s << " (Some other failure)"; break;
            default:
                s << " [unexpected status code]";
        }
        /*switch(status_2){
            case 1: s << " (MINOS needed to many calls for lower error)"; break;
            case 2: s << " (MINOS needed to many calls for upper error)"; break;
            case 3: s << " (MINOS found new minimum in search for lower error)"; break;
            case 4: s << " (MINOS found new minimum in search for upper error)"; break;
            case 5: s << " (Some other MINOS failure)"; break;
            default:
                s << " [unexpected status code]";
        }*/
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
    //overwrite errors with minos errors:
    /*result.errors_plus.set(minos_errors_plus);
    result.errors_minus.set(minos_errors_minus);*/
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

root_minuit::root_minuit(const Configuration & cfg): Minimizer(cfg), tolerance(NAN){
       min.reset(new ROOT::Minuit2::Minuit2Minimizer(type));
       int printlevel = 0;
       if(cfg.setting.exists("printlevel")){
           printlevel = cfg.setting["printlevel"];
       }
       string method = "migrad";
       if(cfg.setting.exists("method")){
           method = (string)cfg.setting["method"];
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
       if(cfg.setting.exists("tolerance")){
           tol = cfg.setting["tolerance"];
       }
       /*if(cfg.setting.exists("minos-errors-for")){
           size_t n = cfg.setting["minos-errors-for"].size();
           for(size_t i=0; i<n; ++i){
               minos_parids.insert(cfg.vm->getParId(cfg.setting["minos-errors-for"][i]));
           }
       }*/
       set_printlevel(printlevel);
       set_tolerance(tol);
       //MinimizerUtils::apply_settings(*this, cfg);
   }


REGISTER_PLUGIN(root_minuit)
