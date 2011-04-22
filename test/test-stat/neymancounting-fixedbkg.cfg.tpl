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
           {type = "flat_distribution"; Theta = {   range = (1E-12, "inf"); fix-sample-value = 1.0; };});
   };
};

mle = {
  type = "mle";
  name = "mle";
  minimizer = { type = "root_minuit"; };
  parameters = ("Theta");
};

writer = {
   type = "pseudodata_writer";
   name = "pd";
   observables = ("o");
   write-data = false;
};


main = {
  data_source = {
     type = "model_source";
     name = "source";
     model = "@counting";
     override-parameter-distribution = {
         type = "product_distribution";
         distributions = ({type = "delta_distribution";  mu = __MU__;}, 
           {type = "equidistant_deltas"; parameter = "Theta"; range = (1E-12, 10.0); n = 1001;});
     };
     //parameters-for-nll = { Theta = "diced_value"; mu = "diced_value"; };
     //rnd_gen = { seed=123;};
  };
  output_database = {
     type = "sqlite_database";
     filename = "neymancounting2.db";
  };

  model = "@counting";
  n-events = 4004000;
  producers = ("@mle", "@writer");
  log-report = false;
};


options = {
   plugin_files = ("$THETA_DIR/lib/core-plugins.so", "$THETA_DIR/lib/root.so");
};

