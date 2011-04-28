For the impatient
-----------------

1. Write all relevant histograms of you analysis in one single ROOT file called "templates.root".
   Include *all* uncertainties by also saving the alternative Histograms affected by +-1sigma uncertainty.
   The histograms must follow the naming convention
   <observable>__<process>[__<uncertainty>__(plus,minus)]
   You can choose the observable, process, and uncertainty names as you like; the observed data must have "DATA" as <process>.
2. call
   theta-auto.py model_summary Higgs
   to produce some summary tables and plots and make sure everything is Ok.
3. To discover a process called 'Higgs', call
   theta-auto.py discover Higgs
   and wait until the toys are done.
   If you have more than 5 sigma, you can skip step 4.
4. If the discovery fails, use
   theta-auto.py exclude_bayesian Higgs
   to derive an upper limit using a Bayesian method and
   theta-auto.py exclude_pp_neyman Higgs
   to derive an upper limit using the Neyman construction where systematic uncertainties are treated in a prior-predictive way.


Introduction
------------

theta-auto.py is a script/API which can be used to automize large parts of some template-based analyses.
It is intended to be simpler and more automized than writing theta cfg-files and analyzer scripts manually. On the other hand,
it is less flexible and can only handle a certain sub-class of models / priors compared to theta.

In these models
 * every observable is a linear combination of templates for different processes
 * all uncertainties are fully correlated across the observables / channels
   (note: the uncertainties among each other can be correlated)
 * the template for one process is a templated morphed with simple_linear_histomorph or cubiclinear_histomorph
 * the coefficient for each (morphed) template are effectively lognormal distributed model parameters

It is important to note that most things in theta-auto are based on naming conventions. You should therefore pay
special attention to understand and implement this naming convention correctly. Otherwise, the result can be very confusing or
frustrating.

Invocation:
theta-auto.py [template-filename.root] [analysis.py] <command> [command-specific options]
[template-filename.root] is the path of a root file containing the templates for the model. It is optional and defaults to "templates.root".
[analysis.py] is the path to an optional python script used to control details of the operation. The default is to use the file 'analysis.py'. If it
   does not exist, it is used as if there was an empty file of that name, i.e., no special options are used.
<command> is the command theta-auto.py will carry out

Each of these three parts will be discussed in more detail below.


template file
-------------
This root file is expected to contain all the templates of the model adhering to a certain naming scheme:
<observable>__<process>[__<uncertainty>__(plus,minus)]

(not that "observable" in theta is a synonym for "channel").

<observable>, <process> and <uncertainty> are names chosen by the user. They must not contain a double underscore.
For example, if you want to make a statistical evaluation of a muon+jets and an electron+jets ttbar cross section measurement,
you can name the observables "mu" and "ele"; the processes might be "ttbar", "w", "nonw", the uncertainties "jes", "q2". Provided
all uncertainties affect all templates, you would supply 6 nominal and 24 "uncertainty" templates:
The 6 nominal would be: mu__ttbar, mu__w, mu__nonw, ele__ttbar, ele__w, ele__nonw
Some of the 24 "uncertainty" histograms would be: mu__ttbar__jes__plus, mu__ttbar__jes__minus, ..., ele__nonw__q2__minus

theta-auto.py expects that all templates of one observable have the same range and binning. All templates have to be normalized
to the same luminosity.

It is possible to omit some of the systematic templates. In this case, it is assumed that the presence of that uncertainty has no
influence on this process in this observable.

Data has the special process name "DATA" (all capitals!), so for each observable, there should be exactly one "<observable>_DATA"
histogram.

To identify which process should be considered as signal, use as process name
"signal". If you have multiple, uncorrelated signal scenarios (such as a new type of particle
with different masses / widths), call it "signal<something>" where <something> is some name you can choose.
Note that for multiple signals, use a number if possible. This number will be used in summary plots if investigating multiple signals.

For example, if you want to search for Higgs with masses 100, 110, 120, ..., you can call the processes "signal100",
"signal110".
Note that this naming convention for signal can be overriden by analysis.py and/or command line options; see below.


constructed model
-----------------

theta-auto will build a model based on the templates where
the systematic uncertainties are fully correlated across different observables and processes, i.e., the same parameter
is used to interpolate between the nominal and shifted templates if the name of the uncertainty is the same. The parameter
for the interpolation is called "delta_<uncertainty>".
Two different systematic uncertainties are assumed to be uncorrelated. Priors for the delta-parameters are
always Gaussians with the special cases of Gaussians with width 0.0 or width inifinity which are effectively delta distributions (used
to fix a parameter) and a flat distribution, respectively.
The delta-parameters have independent Gaussian priors around 0 with width 1 and no truncation. While the parameters are *always* Gaussian,
the mean, width and correlation can be overriden via analysis.py, see below.


analysis.py
------------
There are some global variables and functions available which should be used to specify the options.

the variable 'model' controls aspects of the model:
 * To scale all histograms use  "model.scale_all_histograms(2.0)". This is useful to scale all templates to another luminosity.
 * Specify additional factors for the different processes and channels. These factors are used to encode systematic uncertainties which only influence the 
   rates: use a call "model.set_rate_impact(<process pattern>, <uncertainty>, <rel. uncertainty>)" to set the relative uncertainty this uncertainty has on the rate.
   <process pattern> is a simple pattern (see below) for matching the histogram name, hence selecting one or more (observable, process) combinations.
   
   For example, a 10% theory cross section uncertainty, and two independent 5% lepton efficiency uncertainties (independent for muons and electrons)
   on the 'ttbar' process in the 'electron' and 'muon' observable can be specified by
   model.set_rate_impact('muon__ttbar', 'muon_eff', 0.05)
   model.set_rate_impact('electron__ttbar', 'electron_eff', 0.05)
   model.set_rate_impact('*__ttbar', 'ttbar_theory', 0.1)
   # note: if 'electron' and 'muon' are the only observables, the last line has the same effect as
   # set_rate_impact('muon__ttbar', 'ttbar_theory', 0.2)
   # set_rate_impact('electron__ttbar', 'ttbar_theory', 0.2)
   Internally, rate impacts are modeled by adding a parameter named 'delta_<uncertainty>' with a Gaussian prior around 0 with width 1 (note:
   it is possible to use the same uncertainty name for a rate uncertainty and a shape uncertainty by specifying the same name here!).
   For a configured relative uncertainty r, the actual factor used to scale each process is exp(delta_u * r). With delta_u being Gaussian distributed
   around 0 with width 1, this is equivalent to using a lognormal distributed factor with parameters mu=0 and sigma=r.
   
   For using a truncated Gaussian around 1 with width r instead of a lognormal, use 
   model.set_rate_impact_type_gauss(<uncertainty>)
   So to set the rate uncertainty 'muon_eff' to a truncated gaussian, you can use
   model.set_rate_impact_type_gauss('muon_eff')
   IMPORTANT:
    * this changes the mean, width and range of the associated parameter in model.distribution (see below) such that the parameter can be used
      directly as factor.
    * the change should be made at the very end of the model setup, i.e., after all uncertainties and correlations have been set up
    * this can only be done consistently for rate-only uncertainties which affect all
      processes with the same relative uncertainty. Otherwise, it is not well-defined how to truncate the Gaussian. Trying
      to set the type of an uncertainty to Gaussian for parameters which have different relative rate impact will cause an error.

 * model.distribution is a distribution object for all the parameters, see below for more details.
   For example, you can use
   model.distribution.set_correlation_coefficient('delta_sys1', 'delta_sys2', 0.9)
   to add a +90% correlation for the given parameters (instead of using independent gaussians).
 * model.filter_histograms(l) filters histograms. l is a list of strings (or a single string) representing a pattern used to match histograms names.
   The model is modified as if there would have been only histograms matching at least one of the patterns in the input root file.
   Example: to only use the observable 'muon', you can use
   model.filter_histograms('muon__*')
   It is also possible to only use certain shape uncertainties, say 'jes' and 'q2'. In this case, make sure to keep all the 'nominal'
   histograms as well by including the appropriate pattern:
   model.filter_histograms(['*__*__jes__*', '*__*__q2__*', '*__*'])
 * model.set_signal_processes(l) can be used to set the signal processes. l is a pattern or a list of patterns.
   As default, all processes starting with 'signal' are considered signal (i.e., pattern 'signal*').
 * model.convert_morph_uncertainties_to_rate(): detect cases in which shape uncertainties are actually
   only affecting the rate and can hence be written as a (lognormal) rate uncertainty. This saves computing time.

About patterns: the methods set_rate_impact, filter_histograms, and set_signal_processes accept simple expressions which are used to match
the histogram names in the naming convention. '*' matches any observable/process/uncertainty name.

Distribution objects contain information about the priors for the nuisance parameters. They are always
multivariate gaussians. However, parameters can have 0 width, effectively making
them delta functions, or they can have infinite width and range, effectively making them flat distributions.
 * set_correlation_coefficient('delta_sys1', 'delta_sys2', 0.9): declare a non-zero corr. coeff.
 * set_parameters('delta_sys1', width = 0.3, mean = 1.0, range = (0.0, 2.0)): sets the parameters of the gaussian
   for 'delta_sys1'.
   It is also valid not to specify one of the parameters in which case its value will be unchanged, i.e., a call like
   set_parameters('delta_sys1', width=0.7)
   will only change the width and leave mean and range untouched (which are initialized with 0 and (-infinity, infinity), resp.)


In the configuration, there will be three distributions:
 * model.parameter-distribution: used to generate toy data. Can be controlled via model.distribution in the python config
 * nll_distribution: used to run bayesian methods; defaults to model.parameter-distribution; overrides set via nll_distribution in the python config
 * nll_distribution_minimizers: used to run minimizer-based methods; overrides set via nll_distribution in the python config

functions controlling the behaviour of the commands:
 * usually, the nll is build as the binned likelihood function with additional terms for the prior / constraint of the parameters.
   the global variable "nll_distribution" can be used to specify which distribution will be used in the likelihood definition
   for the method.
   As prior for the nuisance parameters the defaults are
    - the gaussian distrbution given in model.distribution for Bayesian methods
    - a delta distribution (i.e., fixing) to the most probable value for methods using a minimization (likelihood ratio and mle)
   This is useful to let free some parameters in a minimization procedure by setting their width to infinity:
   nll_distribution.set_parameters('delta_sys1', width = float("inf"))
   nll_distribution.set_parameters('delta_sys2', width = float("inf"))
   The prior / constraint for the signal parameter will always be flat.
 * use 'set_option(<option name>, <option value>)' to set a certain option to a certain value. Option names are always
   strings, option values can have different types, depending on the option. For details, see the descriptions
   of commands below.
 * option 'command': the command(s) to execute (see below for a list of commands). Default is to use the command given at the command line.
   Either a string or a list of strings

commands
--------
Valid commands are:
 * model_summary
   summarize the model content: list all detected/configured observables, processes, uncertainties. Make summary tables with:
   - the rates for each process in each channel
   - the rate impact for each process / channel / uncertainty with respect to the nominal one, also indicating whether this is a "shape/rate" uncertainty
     (for which there is an alternative template in the input file) or a "rate only" uncertainty (configured explicitely via set_rate_impact)
   - the correlations and widths for all nuisance parameters
   plots:
   - for each observable, a stack plot with (nominal) predicted and (if available) observed distributions
   - for each triple (observable, process, shape changing uncertainty), one comparison plot of the 'nominal' and shifted templates in both directions
  The plots are saved in the working directory in the "plots" subdirectory as png files.
  The creation of plots can be prevented by using 'set_option("create_plots", False)'
  
 * discover [signal_name]:
    * calculate the likelihood ratio test statistic value on data.
      signal will be fixed to zero in the nominator and fixed to zero in the denominator of the likelihood ratio. 
    * calculate the expected likelihood ratio test statistic distribution using toy Monte-Carlo with the background-only model.
      The required amount of toy MC is estimated by:
       - if there is no previous output in the cache: throw a few thousand toys
       - if there is previous output in the cache: use this data
      In both cases, the toy data is used to extrapolate to the observed value to estimate the required amount of toys.
    * calculate the expected likelihood ratio test statistic distribution for signal + background to quote the "expected" result.
      Can be disabled by using 'set_option("expected_result", False)'
    * If there is more than one signal, theta-auto will use correlated toys to determine and also cite a value for the
      corrected p-value (i.e., corrected for the look-elsewhere effect).
      This can be disabled explcitely with 'set_option("look_elsewhere", False)'.
 * bayesian_limit:
    * Calculates quantiles for the signal cross section marginal posterior. For DATA, also calculate the full marginal posterior.
    * Calculate the expected quantiles using toys. Can be disabled by calling 'set_option("expected_result", False)'
    * set_option("quantiles", [0.16, 0.84]): set different quantiles. Default is 0.95, i.e., 95% C.L. upper limits.
    Plots/numbers produced:
     - for each pair (quantile, signal), show a plot of the expected quantile distribution, indicating the position of the observed one.
       Also write down the p-value.
     - for each quantile, show a plot of the central 68 and 95% of expected quantiles and the observed one. This is only done in case of
       multiple arrangable signals.
       
 * pp_neyman_interval [signal_name]:
    * Calculates (and evaluates) the confidence belt for a Neyman construction with the likelihood ratio as test statistic.
    * Systematic uncertainties are included in a prior-predictive way.
    * use 'set_option("test_statistic", "mle")' to use the maximum likelihood estimate (mle) as test statistic instead of the likelihood ratio
    * use 'set_option("confidence_levels", 0.95)' to change the confidence levels by giving a float or a list of floats. The default is [0.68, 0.95], i.e. 1sigma and 2sigma intervals
    * use 'set_option("ordering_rule", "lower")' to change the ordering rule to include the lowest value of the test statistic first in the belt.
      Other valid options are "upper", "central" and "fc-like" (Feldman-Cousins like). The default is "fc-like".
    Example: for 95% upper limits, you need
    set_option('ordering_rule', 'lower')
    to only calculate the 95% limits (and not 68% ones as well), you can also use
    set_option('confidence_levels', 0.95)
    


In general, many methods produce quite some information as result which can be used in a document. theta-auto.py does this
by writing the result to html files and png figures in the work directory. Point the browser to the work directory to see the index.html file.
   
 * simple_fit
   perform a simple maximum likelihood fit to data. Called "simple" because all delta parameters will be fixed to 0.0, i.e.,
   only the signal will float.
   Writes out the result of the fit and makes plots of all observables normalized to the fit result.
    * perform toy experiments with various amount of data to check the linerity of the fit. Can be disabled with --no-linearity
    * perform toy experiments with toy data with and without uncertainties and produce a table summarizing the (average) influence
      of each systematic uncertainty on the fit result. Can be disabled with --no-uncertainty-scan
 * ks_test
   determine the KS probability for each variable, including systematic uncertainties which are correlated across
   all observables / processes.


more details
------------

This is how theta-auto.py works internally:
 1. a "working directory" is created at the current path, using the python script name (without .py) as directory name, or, if
    the option "workdir" is set in the analysis script, the given path is used instead.
    This directory contains any results produced by theta-auto as specified below.
 2. command-specific theta configuration files are produced in the "cfg" directory. They are set up such that the output
    is written to the "cache" directory. Each configuration file produces an output file of the same name
    (just ending with ".db" instead of ".cfg").
 3. theta is run on all of the configuration files which will produce output in the "cache" directory
 4. the output in the "cache" directory is analyzed and results are derived, possibly producing more intermediate
    files in the "cache" directory (depending on the method)
 
If running theta-auto twice with the same setup, only a part of the work is done: existing configuration files are only
re-generated if the input has changed (similar to make, the decision whether to regenerate the output or not
is based on time-stamps of the dependencies). In particular, theta is only run if the input configuration file changed.
Therefore, running theta-auto again might just trigger timestamp comparisons without any actual calculation.

command line options
---------------------
 * --split=<n>: write config files with a maximum number of n-events = <n>
 * --gencfg: only run theta-auto up to the cfg generation stage, do not run theta.
   This is useful if you want to inspect the generated configuration files or if you know that the calculation
   is long and you want to run theta on your own on a cluster. In this case
   - use --gencfg (and probably --split) to generate the config files
   - run theta on all the configuration files in "cfg" on the cluster
   - copy the resulting db files (one per theta invocation) to the "cache" directory
   - run theta-auto again, this time omitting the "--gencfg" option. theta-auto should recognize that
     the output is newer than the config files and not run theta and start with deriving results immediately.
