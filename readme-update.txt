1. globally defined "parameters" setting is now only a list of names
2. what used to be "ranges" and "default" in the "parameters" setting is now at the model or producer level:
  a parameter-distribution setting in the model MUST be defined ) which contains
  a prior distribution for ALL parameters of the model (which can be flat).
  This also replaces the "constraints" setting in the model.
  (this is to minimize global information. It should make the process of pseudo data generation more transparent
  to the user, at least in more complex settings; see also 4. below.)
3. the component specification setting used to have a "coefficients" setting. For
  more consistency, this is replaced by a "coefficient-function"  setting which has
  to specify a function.
  Change "coefficients = (<list>);" to "coefficient-function = {type = "mult"; parameters = (<list>);};"
4. run now contains a data-source setting to specify where to take the data from.
  Use
  data-source = { type = "model_source"; model = "@..."; };
  for now where model is the model you want to draw the pseudo data from. The only model specified in the run
  is now simply called "model" and is now what was "model-producers" before.
  (the introduction of data-source can be used to write data-providing plugins, for example
  to run over real data or to run over special test pseudo data).
5. For deltanll_hypotest and mcmc_posterior_ratio, the signal-plus-background and background-only
  hypotheses are now specified as distributions which each have to specify ALL model parameters. Typically,
  you can use
  signal-plus-background-distribution = "@path-to-model.parameter-distribution";
  background-only-distribution = "@b-dist"; //some globally defined product_distribution
  

other changes:
* parameters used for pseudo data generation are now in the "events" table, not 
  in their own table any more
* for all producers, it is possible to specify additional likelihood functions to add
  (as additional prior, etc.)
* configuration links to nested setting paths are now possible ("@path1.path2.path3")

