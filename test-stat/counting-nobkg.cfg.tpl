//note: this is not a valid configuration file, but a configuration file template.
//It is used by the test scripts which do some replacements, e.g., __THETA__ is replaced by a flating point value.

parameters = {
   Theta = {
        default = __THETA__;
        range = (0.0, "inf");
   };
};

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

//model definition:
counting = {
   o = {
      signal = {
          coefficients = ("Theta");
          histogram = "@flat-unit-histo";
      };
   };
};

