#include "interface/phys.hpp"

/** \brief A Function which multiplies different factors which can be parameters, literal values and other Functions
 *
 * It is a generalization of the \link mult mult \endlink plugin and accepts more types of factors, namely
 * constant values and other Functions.
 *
 * Example configuration:
 * \code
 * function1 = {
 *   type = "multiply";
 *   factors = ("p1", "p2", 1.7, 2.3, "@function2");
 * }
 * ...
 * function2 = {
 *   type = "multiply"; factors = ("p3", 2.2);
 * };
 * \endcode
 *
 * \c factors is a list of factors. It can be a literal floating point value will be included directly as factor,
 *   a string which is interpreted as parameter name which will be included in the multiplication or a setting group
 *   used to construct a Function object via the plugin system.
 *
 * Note: Specifying literal values is mainly useful in conjunction with using the "@"-notation to refer to one 'global' parameter.
 * For example, you can use a global setting 'lumi' set to the current luminosity under study and include the factor "@lumi" in all
 * histogram coefficients factors in the model.
 */
class multiply: public theta::Function{
public:
    /** \brief Construct a MultFunction from a Configuration instance
     */
    multiply(const theta::plugin::Configuration & cfg);
    virtual double operator()(const theta::ParValues & v) const;    
private:
    std::vector<theta::ParId> v_pids;
    double literal_factor;
    boost::ptr_vector<theta::Function> functions;
};

