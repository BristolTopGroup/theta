#include "interface/phys.hpp"
#include "interface/plugin.hpp"

/** \brief A data source using a model, but fixed average data instead of Poisson random
 *
 * Configured via a setting group like
 * \code
 * source = {
 *   type = "model_source_norandom";
 *   model = "@some-model-path";
 *   parameter-values = (("s", 10.0), ("b", 5.0));
 * };
 * \endcode
 *
 * \c type must be "model_source_norandom" to select this DataSource type
 *
 * \c model specifies the model to use for data creation
 *
 * \c parameter-values is a list of lists specifying the parameter values. You have to specify values for
 *    all model parameters.
 *
 * model_source_norandom will not create any columns in the products table.
 */
class model_source_norandom: public theta::DataSource{
public:

    /// Construct from a Configuration; required by the plugin system
    model_source_norandom(const theta::plugin::Configuration & cfg);

    /** \brief Fills the provided Data instance with data from the model
     *
     * Will never throw the DataUnavailable Exception.
     */
    virtual void fill(theta::Data & dat, theta::Run & run);
    
    virtual void define_table(){}

private:
    theta::Data data;
    theta::ParValues values;    
    std::auto_ptr<theta::Model> model;
};
