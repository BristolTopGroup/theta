parameters = ("Theta", "mu");

observables = {
   o = {
      nbins = 1;
      range = [0.0, 1.0];
   };
};


flat-unit-histo = {
    type = "fixed_poly";
    observable = "o";
    normalize_to = 1.0;
    coefficients = [1.0];
};

counting = {
   o = {
      signal = {
          coefficient-function = { type = "mult"; parameters = ("Theta");};
          histogram = "@flat-unit-histo";
      };
      background = {
          coefficient-function = { type = "mult"; parameters = ("mu");};
          histogram = "@flat-unit-histo";
      };
   };
   
   parameter-distribution = {
      type = "product_distribution";
      distributions = ({type = "delta_distribution";  mu = __MU__;}, 
           {type = "flat_distribution"; Theta = {   range = ("-inf", "inf"); fix-sample-value = 1.0; };});
   };
};

/*mle = {
  type = "mle";
  name = "mle";
  minimizer = { type = "root_minuit"; };
  parameters = ("Theta");
};*/

writer = {
   type = "pseudodata_writer";
   name = "pd";
   observables = ("o");
   write-data = false;
};

main = {
  type = "fclike_ordering";
  output_database = {
     type = "sqlite_database";
     filename = "ordering.db";
  };

  model = "@counting";
  parameter_of_interest = "Theta";
  parameter_range = [-2.99, 29.99];
  n_parameter_scan = 330;
  ts_producer = "@writer";
  ts_name = "n_events_o";
};


options = {
   plugin_files = ("$THETA_DIR/lib/core-plugins.so", "$THETA_DIR/lib/root.so");
};

