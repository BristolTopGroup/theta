#ifndef PLUGINS_FCLIKE_ORDERING
#define PLUGINS_FCLIKE_ORDERING

#include "interface/decls.hpp"
#include "interface/main.hpp"
#include "interface/plugin.hpp"
#include "interface/variables.hpp"

/** \brief Calculate an ordering grid for a Neyman construction on the (truth, test statistic) plane inspired by Feldman / Cousins
 *
 * Configured via a setting like
 * \code
 * main = {
 *   type = "fclike_ordering";
 *   model = "@some_model";
 *   output_database = {
 *       type = "sqlite_database";
 *       filename = "ordering.db";
 *   };
 *   //information about the "truth" axis:
 *   parameter_of_interest = "beta_signal"; // assuming beta_signal was declared as parameter
 *   parameter_range = [0.0, 5.0];
 *   n_parameter_scan = 500;
 *   // information about "test statistic" axis:
 *   ts_producer = "@my-mle";   //typical choices: producer of type mle or producer of type lr
 *   ts_name = "beta_signal"; //product name / column name of the test statistic
 * };
 * \endcode
 *
 * The output database will contain a table named 'ordering' with the columns 'truth', 'ts', 'ordering' containing the true value
 * of the parameter of interest, the value of the test statistic and the likelihood ratio value to be used for ordering for this (truth, ts)
 * value pair.
 * The number of rows is n_parameter_scan^2.
 *
 * The reported progress is the number of parameter scan points in one dimension.
 */
class SaveDoubleProducts;
class fclike_ordering: public theta::Main{
public:
    fclike_ordering(const theta::plugin::Configuration & cfg);
    virtual void run();
private:
   std::auto_ptr<theta::Model> model;
   boost::shared_ptr<theta::Database> database;
   boost::shared_ptr<SaveDoubleProducts> save_double_products;
   
   theta::ParId poi;
   double poi_min, poi_max;
   int n_poi_scan;
   
   std::auto_ptr<theta::Producer> ts_producer;
   std::string ts_name;
};

#endif

