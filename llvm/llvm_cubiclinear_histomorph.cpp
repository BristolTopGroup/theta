#include "llvm/llvm_cubiclinear_histomorph.hpp"
#include "llvm/llvm_interface.hpp"
#include "interface/plugin.hpp"

#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/Support/IRBuilder.h"

using namespace theta;
using namespace llvm;

void llvm_cubiclinear_histomorph::apply_functor(const theta::functor<theta::Histogram1D> & f, const theta::ParValues & values) const{
	h = h0;
	const size_t n_sys = hplus_diff.size();
    for (size_t isys = 0; isys < n_sys; isys++) {
        const double delta = values.get(vid[isys]) * parameter_factors[isys];
        if(delta==0.0) continue;
        //linear extrapolation beyond 1 sigma:
        if(fabs(delta) > 1){
            const Histogram1D & t_sys = delta > 0 ? hplus_diff[isys] : hminus_diff[isys];
            h.add_with_coeff(fabs(delta), t_sys);
        }
        else{
            //cubic interpolation:
            const double d2 = delta * delta;
            const double d3 = d2 * fabs(delta);
            h.add_with_coeff(0.5*delta, diff[isys]);
            h.add_with_coeff(d2 - 0.5 * d3, sum[isys]);
        }
    }
    double h_sum = 0.0;
    for(size_t i=0; i < h.get_nbins(); ++i){
        double val = h.get(i);
        if(val < 0.0){
            h.set(i, 0.0);
        }
        else{
            h_sum += val;
        }
    }
    if(normalize_to_nominal && h_sum > 0.0){
       h *= h0_sum / h_sum;
    }
    f(h);
}

llvm::Function * llvm_cubiclinear_histomorph::llvm_codegen(llvm_module & mod, const std::string & prefix) const{
	// 1. create global data; most of it const, but one buffer for writing the
	// intermediate result (required for 0-truncation)
	size_t nbins = h0.get_nbins();
	// 1.a. constant global data:
	std::vector<GlobalVariable*> llvm_hplus_diff;
	std::vector<GlobalVariable*> llvm_hminus_diff;
	std::vector<GlobalVariable*> llvm_diff;
	std::vector<GlobalVariable*> llvm_sum;
	const size_t npars = hplus_diff.size();
	theta_assert(hminus_diff.size() == npars);
	theta_assert(diff.size() == npars);
	theta_assert(sum.size() == npars);
	GlobalVariable* llvm_h0 = add_global_const_ddata(mod.module, prefix + "h0", h0.get_data(), nbins);
	for(size_t i=0; i<npars; ++i){
		std::stringstream ss;
		ss << i;
		llvm_hplus_diff.push_back(add_global_const_ddata(mod.module, prefix + "hplus_diff" + ss.str(), hplus_diff[i].get_data(), nbins));
		llvm_hminus_diff.push_back(add_global_const_ddata(mod.module, prefix + "hminus_diff" + ss.str(), hminus_diff[i].get_data(), nbins));
		llvm_diff.push_back(add_global_const_ddata(mod.module, prefix + "diff" + ss.str(), diff[i].get_data(), nbins));
		llvm_sum.push_back(add_global_const_ddata(mod.module, prefix + "sum" + ss.str(), sum[i].get_data(), nbins));
	}
	// 1.b. add "h":

	/*std::vector<Constant*> elements(n);
	for(size_t i=0; i<n_orig; ++i){
		elements[i] = ConstantFP::get(m->getContext(), APFloat(data[i]));
	}
	if(n_orig % 2){
		elements[n_orig] = ConstantFP::get(m->getContext(), APFloat(0.0));
	}
	ArrayType* typ = ArrayType::get(Type::getDoubleTy(m->getContext()), n);
	Constant * ini = ConstantArray::get(typ, elements);
	GlobalVariable * gvar = new GlobalVariable(*m, typ, true, GlobalValue::PrivateLinkage, ini, name);
	gvar->setAlignment(16);*/

	// 2. create function and read arguments:
	FunctionType * FT = get_ft_function_evaluate(mod);
	llvm::Function * F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, prefix + "_evaluate", mod.module);
	llvm::Function::arg_iterator it = F->arg_begin();
	Value * coeff = it++;
	Value * par_values = it++;
	Value * data = it++;
	theta_assert(it== F->arg_end());

	// 3. build function body
	LLVMContext & context = mod.module->getContext();
	IRBuilder<> Builder(context);
	BasicBlock * BB = BasicBlock::Create(context, "entry", F);
	Builder.SetInsertPoint(BB);



	Type * double_t = Type::getDoubleTy(context);
	Type * i32_t = Type::getInt32Ty(context);

	return F;
}

void llvm_cubiclinear_histomorph::get_histogram_dimensions(size_t & nbins, double & xmin, double & xmax) const{
    nbins = h0.get_nbins();
    xmin = h0.get_xmin();
    xmax = h0.get_xmax();
}


llvm_cubiclinear_histomorph::llvm_cubiclinear_histomorph(const Configuration & ctx): normalize_to_nominal(false) {
    boost::shared_ptr<VarIdManager> vm = ctx.pm->get<VarIdManager>();
    //build nominal histogram:
    h0 = get_constant_histogram(Configuration(ctx, ctx.setting["nominal-histogram"])).get_values_histogram();
    if(ctx.setting.exists("normalize_to_nominal")){
        normalize_to_nominal = ctx.setting["normalize_to_nominal"];
    }
    Setting psetting = ctx.setting["parameters"];
    size_t n = psetting.size();
    parameter_factors.resize(n, 1.0);
    bool have_parameter_factors = ctx.setting.exists("parameter_factors");
    for(size_t i=0; i<n; i++){
        std::string par_name = psetting[i];
        ParId pid = vm->get_par_id(par_name);
        par_ids.insert(pid);
        vid.push_back(pid);
        std::string setting_name;
        //plus:
        setting_name = par_name + "-plus-histogram";
        hplus_diff.push_back(get_constant_histogram(Configuration(ctx, ctx.setting[setting_name])).get_values_histogram());
        hplus_diff.back().check_compatibility(h0);
        hplus_diff.back().add_with_coeff(-1.0, h0);
        //minus:
        setting_name = par_name + "-minus-histogram";
        hminus_diff.push_back(get_constant_histogram(Configuration(ctx, ctx.setting[setting_name])).get_values_histogram());
        hminus_diff.back().check_compatibility(h0);
        hminus_diff.back().add_with_coeff(-1.0, h0);

        sum.push_back(hplus_diff.back());
        sum.back() += hminus_diff.back();
        diff.push_back(hplus_diff.back());
        diff.back().add_with_coeff(-1, hminus_diff.back());

        if(have_parameter_factors){
            parameter_factors[i] = ctx.setting["parameter_factors"][i];
        }
    }
    h0_sum = h0.get_sum();
    h = h0;
}

REGISTER_PLUGIN(llvm_cubiclinear_histomorph)
