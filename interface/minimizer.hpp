#ifndef MINIMIZER_HPP
#define MINIMIZER_HPP

#include "interface/variables.hpp"
#include "interface/matrix.hpp"
#include "interface/phys.hpp"
//#include "interface/plugin_so_interface.hpp"

#include <map>

namespace theta{

    /** \brief The result of a minimization process, returned by \c Minimizer::minimize().
     */
    struct MinimizationResult{
        
        /** \brief The function value at the minimum.
         */
        double fval;

        /** \brief The parameter values at the function minimum.
         *
         * Contains exactly the parameters the function to minimize depends
         * on.
         */
        ParValues values;

        /** \brief The errors at the function minimum, in positive direction.
         *
         * Contains all parameters the function to minimize depends
         * on. How this is calculated depends on the minimizer used. In cases where
         * the minimizer provides symmetrical errors, the entries are equal to \c errors_minus.
         *
         * Set to -1 if not provided by the minimizer.
         */
        ParValues errors_plus;

        /** \brief The errors at the function minimum, in negative direction.
         *
         * Contains all parameters the function to minimize depends
         * on. How this is calculated depends on the minimizer used. In cases where
         * the minimizer provides symmetrical errors, the entries are equal to \c errors_plus.
         *
         * Note that while these are the errors in negative direction, the
         * entries are always positive in case it contains valid errors.
         *
         * Set to -1 if not provided by the minimizer.
         */
        ParValues errors_minus;

        /** \brief Contains the error matrix at the minimum.
         *
         * It is quadratic and has values.size() rows.
         * The index convention is such that 0 corresponds to the first ParId in
         * the sorted (=as iterated) list of parameter ids contained in \c values.
         *
         * It is the negative unity matrix of the correct size in case the
         * minimization does not provide errors.
         */
        Matrix covariance;
    };


    /** \brief Abstract interface to different minimizers.
     *
     * Configuration through a setting like:
     *  <pre>
     * {
     * type = "...";
     * override-ranges = { //optiona;
     *     some_param_name = (0.0, 2.0);
     * };
     * initial-step-sizes = { //optional
     *     some_param_name = 0.1;
     * };
     * };
     * </pre>
     *
     * The type setting must be set to an available Minimizer name.
     * 
     * \c edm-tolerance is used in a call to Minimizer::set_edm_tolerance, see documentation there.
     *
     * \c fval-tolerance is used in a call to Minimizer::set_fval_tolerance, see documentation there.
     *
     * \c override-ranges override the default ranges of parameters. See Minimizer::override_range.
     *
     * \c initial-step-sizes sets initial step sizes for the parameters. If no initial step sizes are given here,
     * it is chosen with rules documented in Minimizer::get_initial_stepsize.
     *
     * Note that many more settings might be available (or necessary) for special
     * types of minimizers. For documentation of these settings, see the
     * Factory classes of these derived classes from Minimizer.
     */
    class Minimizer{
    public:
        typedef Minimizer base_type;

        // declare destructor virtual as we expect polymorphic access to derived classes
        virtual ~Minimizer(){}

        /** Attempts to minimize the function.
         *
         * If a serious error occurs during minimization and the minimization fails,
         * a MinimizationException is thrown. The reasons for such a failure are manifold
         * and also depend on the particular minimization algorithm.
         */
        virtual MinimizationResult minimize(const theta::Function & f) = 0;

        /** \brief Override the allowed range used for a particular parameter.
         *
         * Setting limits of parameter \c p ensures that the
         * function is never evaluated at points for which parameter \c p is outside
         * the provided range.
         *
         * By setting the exact same \c lower_limit and \c upper_limit, you can fix the parameter.
         *
         * It is invalid to call the function with upper_limit &lt; lower_limit. In
         * this case, an InvalidArgumentException will be thrown.
         *
         * Usually, the allowed range is defined hrough parameter definition
         * (i.e., within \c VarIdManager). Calling this function will override
         * this default behaviour and apply other limits.
         *
         * Note that ranges are usually enforced by applying a non-linear parameter transformations
         * to the parameters. This can make the minimization problem more difficult than it was without
         * the limit. Therefore, you should always avoid ranges at all or choose
         * the as large as possible.
         */
        void override_range(const theta::ParId & pid, double lower_limit, double upper_limit);

        /** \brief Resets a range override via \c override range.
         *
         * After a call of \c override_range, you can call this function to
         * use the default range instead.
         * 
         * It is valid to call this function with ParId values \c pid
         * even if there was no previous call of \c override_range before.
         * So if you want to make sure that there are no range overrides
         * at all, you can call this function for all \c pid values the
         * function to be minimized depends on.
         */
        void reset_range_override(const theta::ParId & pid);

        /** \brief Set an initial stepsize for parameter \c pid.
         *
         * The supplied stepsize has to be stricly larger than zero. Otherwise,
         * an InvalidArgumentException will be thrown.
         */
        void set_initial_stepsize(const theta::ParId & pid, double stepsize);

        /** \brief Forget about the previously set stepsize via \c set_initial_stepsize.
         *
         * See also the comments of \c reset_range_override; they apply here correspondigly.
         */
        void reset_initial_stepsize(const theta::ParId & pid){
            initial_stepsizes.erase(pid);
        }

    protected:
        const boost::shared_ptr<theta::VarIdManager> vm;

        double tolerance;

        /** \brief Get the currently valid range of parameter \c pid.
         *
         * Returns the range set through \c override_range or (if there was no
         * call for this \c pid) the range from the VarIdManager \c vm.
         */
        std::pair<double, double> get_range(const theta::ParId & pid) const;

        /** \brief Get the currently valid initial setsize of parameter \c pid.
         *
         * Analogue of \c get_range for stepsizes.
         *
         * If no initial stepsize was set, returns 20% of the absolute default value of
         * the parameter, if the latter is > 0.0. Otherwise, a 5% of the parameter range
         * will be returned. If the range has infinite length, 1.0 will be used.
         */
        double get_initial_stepsize(const theta::ParId & pid) const;
        
        Minimizer(const boost::shared_ptr<theta::VarIdManager> & vm_): vm(vm_){}

    private:
        std::map<theta::ParId, std::pair<double, double> > overridden_ranges;
        std::map<theta::ParId, double> initial_stepsizes;
    };


    namespace MinimizerUtils{
        /** \brief Evaluate the configuration common to all minimizers, i.e.,
         * \c override-ranges and \c initial-step-sizes and passes them to \c m by calling
         * the appropriate methods.
         *
         * This function is usually called only from a derived class of MinimizerFactory.
         */
        void apply_settings(Minimizer & m, theta::plugin::Configuration & ctx);
    }

}


#endif /* _MINIMIZER_HPP */

