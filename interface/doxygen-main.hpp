/* this file is not for inclusion in C++ code. It is only here
 * to provide a "mainpage" block for doxygen (and I didn't want to spoil some random
 * header with it ...). It also includes documentation of the "theta" namespace
 * which is distributed over many header files.
 */

/** \mainpage
 *
 * Welcome to %theta. %theta is a framework for statistical tests, focussing on problems
 * in high-energy physics. It provides the possibility for the user to express a "model", i.e.,
 * the expected data distribution, as function of physical parameters. This model can be used
 * to make statistical inference about the physical parameter of interest.
 * %theta make it easier to treat commonly arising tasks such as the treatment of nuisance
 * parameters, coverage tests, luminosity scans, large-scale production of test statistic points,
 * and many more.
 *
 * The documentation is split into several pages. If you are new to %theta, read them in this order:
 * <ol>
 *   <li>\subpage whatistheta "What theta can do" explains which kind of questions are addressed with %theta</li>
 *   <li>\subpage installation Installation explains how to obtain and compile %theta</li>
 *   <li>\subpage intro Introduction describes how to run %theta; a first example is discussed and
 *        an introduction to the internals of %theta. In also contains a \ref plugins "list of available plugins".</li>
 *   <li>\subpage cmd_interface "Command line interface" described the command line tools of %theta</li>
 *   <li>\subpage design "Design Goals of theta" contains some thoughts about what the code of %theta should be like.
 *       You should read that either if you want to contribute code to %theta or if you want to know what makes %theta
 *       different to other software you often deal with in high-energy physics.</li>
 * </ol>
 *
 * %theta contains examples in the \c examples directory.
 *
 * %theta logos are available here as <a href="../logos/theta.pdf">pdf</a>, <a href="../logos/theta.eps">eps</a> and
 * <a href="../logos/theta.png">png</a>.
 *
 * \section ack Acknowledgement
 *
 * I would like to thank the authors of the excellent software packages used by %theta:
 * <ul>
 * <li><a href="http://www.hyperrealm.com/libconfig/libconfig.html">libconfig</a> A
 *      well-written, well-documented C/C++ library for processing configuration files with a very simple and elegant API.</li>
 * <li><a href="http://www.chokkan.org/software/liblbfgs/">liblbfgs</a> An implementation of
 *     the Limited-memory Broyden-Fletcher-Goldfarb-Shanno minimization algorithm</li>
 * </ul>
 * These libraries are included in the distribution of %theta.
 *
 * Furthermore, some parts of numerical algorithms have
 * been copied from the excellent <a href="http://www.gnu.org/software/gsl/">GNU Scientific Library (GSL)</a>.
 *
 * Last but not least, I want to thank Jasmin Gruschke who tested %theta from an end-user point of view, made useful
 * suggestions and bravely endured many backward-incompatible changes.
 *
 * \section license License
 *
 * %theta is licensed under the <a href="http://www.gnu.org/copyleft/gpl.html">GPL</a>.
 *
 * \section ref References
 * %theta includes algorithms based on:
 * <ol>
 *   <li><em>Matsumoto, Makoto and Nishimura, Takuji:</em> <a href="http://doi.acm.org/10.1145/272991.272995">"Mersenne twister: a 623-dimensionally equidistributed uniform pseudo-random number generator"</a>,
 *        ACM Trans. Model. Comput. Simul. 1, 1998</li>
 *   <li><em>Pierre L'Ecuyer:</em> "Maximally Equidistributed Combined Tausworthe Generators", Math. Comp. 65, 1996</li>
 *   <li><em>Pierre L'Ecuyer:</em> "Tables of Maximally Equidistributed Combined LFSR Generators", Math. Comp. 68, 1999</li>
 *   <li><em>George Marsaglia and Wai Wan Tsang:</em> "The Ziggurat Method for Generating Random Variables", Journal of Statistical Software 8, 2000</li>
 *   <li><em>A. Gelman, G. O. Roberts, and W. R. Gilks:</em> "Efficient Metropolis Jumping Rules", Bayesian Statistics 5, 1996</li>
 * </ol>
 */


/** \page whatistheta What %theta can do
 *
 * %theta is about modeling and statistical inference. For %theta, "model" means
 * a specification of the probability density of one or more observables as function of
 * some parameters, including possible probability densities of the parameters.
 *
 * In %theta, models are <em>always</em> given as a linear combination of templates (i.e., histograms), where
 * both the coefficients and the templates depend in general on the parameters of the model. %theta uses this
 * model for both, pseudo data creation and to make statistical inferences.
 *
 * %theta uses a plugin system which enables you to easily extend %theta. For example, you can define
 * your own parameter-dependend template or template coefficient. But you can also use one of the already supplied
 * methods for these tasks, including
 * <ul>
 *  <li>reading histograms from a ROOT file</li>
 *  <li>specifying a histogram by a polynomial or gaussian</li>
 *  <li>specifying a histogram interpolation (sometimes called "morphing") which  uses model parameters
 *     to interpolate between a "nominal" and several "distorted" templates. This can be used as generic model to
 *     treat systematic uncertainties</li>
 * </ul>
 *
 * Some typical statistical questions which can be addresses are (of course, this can be done
 * with the full modeling available, e.g., for models using template interpolation to treat systematic uncertainties):
 * <ul>
 * <li>Determine the quantiles of the marginal posterior using markov chains. This can be used for
 *    upper limits or symmetrical credible intervals.</li>
 * <li>Determine the marginal posterior density, given data.</li>
 * <li>Dicovery or observation in a signal search by create large-scale
 *   likelihood ratio test statistic to find out the critical region of
 *   a hypothesis test</li>
 * <li>Run MINUIT minimization and error estimation on the negative log-likelihood to estimate a
 *    parameter (cross section or other); make pseudo experiments as consistency check and to cite the expected
 *    statistical and systematic uncertainty.</li>
 * </ul>
 *
 *
 * \section boostedtop Z' search
 *
 * A complete analysis example which makes use of many features of %theta is currently in preparation.
 * However, to get an impression of what can be done, an example analysis searching for \f$ Z^\prime \rightarrow
 * t \bar t \f$ in the semileptonic muon channel
 *  at Z' masses in the TeV regime at the LHC. For some background, you can read the
 * <a href="http://cdsweb.cern.ch/record/1194498/files/EXO-09-008-pas.pdf">CMS PAS 2009-09-008</a>, but it is
 * not required for further reading. Note that %theta was not
 * used for the results presented there, but it was designed for such a case.
 *
 * In this analysis, after the event selection, the invariant mass of the ttbar system, \f$ M_{t\bar t} \f$,
 * is estimated event-by-event. For a fixed Z' mass, a Monte-Carlo model predicts this \f$ M_{t\bar t} \f$ distribution
 * for signal. For the background distribution, usually Monte-Carlo templates are used, the main backgrounds being
 * vector-boson (W,Z) + jets, QCD, and standard-model \f$ t\bar t\f$.
 * As the QCD processes is assumed to be not well modeled by Monte-Carlo,
 * its shape is extracted from a background-enriched data sideband. To determine an upper limit for a fixed Z' mass,
 * two variables are fitted simultaneously: \f$ H_{T}^{lep} \f$, the scalar sum of missing \f$ E_T \f$ and muon \f$ p_T \f$,
 * and the reconstructed invariant top-quark pair mass, \f$ M_{t\bar t} \f$. By fitting both templates simultaneously,
 * the QCD background normalization can be extracted from data instead on relying on Monte-Carlo prediction.
 *
 * So far, the model (and therefore the liklihood built from this model, given data), depends on
 * 4 parameters: (i) the vector-boson + jets mean, (ii) the standard model \f$ t\bar t\f$ mean,
 * (iii) the QCD mean and (iv) the signal mean. All these parameters are mean values of the expected number of
 * events after the event selection. They can be translated to a cross section, given the selection acceptance and
 * integrated luminosity. It is assumed that inferences about the mean values after selection can be transformed
 * to statements about the cross sections easily. Two of these parameters, namely the rate for
 * vector-boson + jets and standard model \f$ t\bar t\f$, are assumed to be fairly well modeled by Monte-Carlo simulation.
 * However, instead of fixing these parameters in the model to their predicted value, some degree of mismodeling is
 * accounted for by treating the parameter as free parameter in the likelihood function but adding
 * a Gaussian term keeping it close to the predicted one where the width corresponds to the uncertainty of the prediction
 * (Bayesians would call this "prior" but this constraint can also be justified as part of the model freqentistically).
 *
 * The situation is further complicated by the presence of different systematic uncertainties which affect the
 * templates used in the model in both the shape and the rate. For example, jet energy scale uncertainty,
 * theoretical uncertainties like scale uncertainty, modeling of initial- and final-state radiation and more.
 *
 * These systematic uncertainties are incorporated in the model by template interpolation described above. This introduces
 * one additional parameter per systematic uncertainty. Assuming we have 3 systematic uncertainties
 *
 * \subsection boostedtop_upperlimit Upper limit
 *
 * Once the templates have been built, %theta can be used to perform this fit and extract the upper limit using
 * a fully Bayesian method. This method constructs the poterior density and uses a Markov-Chain Monte-Carlo
 * method to find the 95% quantile of the signal cross-section marginal posterior.
 *
 * This can be used to compute the median upper limit expected in the case that there is no Z'.
 *
 * \subsection boostedtop_discovery Discovery and frequentist intervals
 *
 * The same model can be used to produce likelihood ratio test stistics distribution which, once created, can easily
 * be used to find out the critical region of a hypothesis test attempting to find signal.
 *
 * The likelihood ratio in %theta is defined by the ratio of two likelihood values obtained for two different
 * special cases of the same model: the "signal-plus-background" case and the "background-only" case. It is assumed
 * that these special cases can be expressed by fixing parameters in the more general model.
 * The likelihood value used for the ratio is then found by minimizing with respect to all
 * other (i.e., non-fixed) parameters.
 *
 * However, as numerical is hard in case of many free parameters (what easily happens if adding more and more systematic
 * uncertainties), %theta also supports the creation of an alternate likelihood-ratio statistic which integrates over all
 * non-fixed parameters instead of minimizing.
 *
 * If scanning through the signal parameter, this likelihood-ratio test statistic can also be used to construct frequentist
 * confidence intervals (including upper limits).
 *
 * Note that, however, %theta so far only assists in the large-scale creation of test statistics, not in the
 * subsequent statistical inferences making use of it.
 */


/** \page installation Installation
 *
 * \section obtaining Obtaining theta
 *
 * %theta is available as source-code distribution via subversion only. The latest version can be obtained by running
 * <pre>
 * svn co https://ekptrac.physik.uni-karlsruhe.de/svn/theta/trunk theta
 * </pre>
 * If you want to compile it within CMSSW (which provides the dependencies), make sure to check it
 * out in the \c src directory of a CMSSW directory.
 *
 * \section building Building theta
 *
 * \subsection with_cmssw With CMSSW
 *
 * Go to the \c theta directory and run \c make. Before running the main executable \c bin/theta, execute
 * <pre>
 * source setenv.sh
 * </pre>
 * from the installation root to adapt \c LD_LIBRARY_PATH for the shared objects of boost and sqlite3. This ensures
 * that the ones from the CMSSW distribution will be used (with which it was compiled).
 *
 * If you want to test that everything is Ok, you can run the unit-tests provided with %theta, by running
 * <pre>
 * make run-test
 * </pre>
 *
 * So a complete set of commands to check out, compile, test and run %theta with CMSSW
 * would be (assuming that cmssw was set up):
 * <pre>
 *  scram project CMSSW CMSSW_3_5_0
 *  cd CMSSW_3_5_0/src
 *  cmsenv
 *  svn co https://ekptrac.physik.uni-karlsruhe.de/svn/theta/trunk theta
 *  cd theta
 *  make
 *  source setenv.sh
 *  make run-test
 *  bin/theta examples/gaussoverflat.cfg
 * </pre>
 *
 * <tt>CMSSW_3_5_0</tt> is an example, you can poick another version. It is recommended to pick a recent one, as %theta
 * was not tested with older versions of CMSSW which contain older versions of boost which %theta depends on. However,
 * your chances are good that it compiles as well under older versions.
 *
 * \subsection without_cmssw Without CMSSW
 *
 * Be sure to get the external dependencies:
 * <ol>
 * <li><a href="http://boost.org/">Boost</a>, a bundle of general-purpose C++ libraries, which is used in %theta for filesystem interface, 
 *    option parsing, memory management through smart pointers, and multi-threading.</li>
 * <li><a href="http://sqlite.org/">sqlite3</a>, a light-weight, SQL-based %database engine which is used to
 *    store the results in %theta</li>
 * </ol>
 * There are packages available for these on many distribution.
 *
 * Then, you can follow the instructions above, skipping the CMSSW part:
 *<pre>
 *  svn co https://ekptrac.physik.uni-karlsruhe.de/svn/theta/trunk theta
 *  cd theta
 *  make
 *  source setenv.sh
 *  make run-test
 *  bin/theta examples/gaussoverflat.cfg
 *</pre>
 * If you install the dependencies in an unusual location (i.e., if headers and libraries are not found automatically)
 * you have to edit the SQLITE_INCLUDE, SQLITE_LIBS, BOOST_INCLUDE, and BOOST_LIBS variables on top of \c Makefile.rules.
 */

 /**
 * \page intro Introduction

 * This page discusses  a concrete example where you get an overview over how %theta works
 * from the point of view of a user.
 * In the second section, some internals of %theta are explained which are good to know even if you
 * do not plan to extend %theta.
 *
 * \section first_example First example
 *
 * Suppose you search for a new particle. After a sophsticated event selection,
 * you have events containing candidates of your new particle.
 * For each of these events, you can reconstruct the mass. From your Monte-Carlo simulation,
 * you conclude that your signal has a distribution in this reconstructed mass in the
 * form of a gaussian with mean \f$ 1000\,\mathrm{GeV}/c^2 \f$ and width \f$ 250\,\mathrm{GeV}/c^2 \f$, whereas your background
 * is expected to be flat in the region from 500 to \f$ 1500\,\mathrm{GeV}/c^2 \f$ which should be used to further constrain
 * your background.
 *
 * You do the studies mainly at one fixed integrated luminosity L. From a background fit
 * to a sideband you expect that you can constrain your background poisson mean in the signal
 * region to \f$ 1600 \pm 200 \f$ events. The model of your signal allows for a large variety of signal
 * cross sections; the standard model predicts no "signal".
 *
 * For this introduction, we consider the creation of likelihood-ratio test statistic distribution for the
 * "background only" null hypothesis in order to find out the critical region for the hypothesis test attempting
 * to reject the null hypothesis at some confidence level.
 *
 * Analysis with %theta always consists of several steps, namely:
 *<ol>
 * <li>Model definition</li>
 * <li>Configuration of the statistical methods to apply</li>
 * <li>Configuration of the run</li>
 * <li>Executing the %theta main program (optional: more than once)</li>
 * <li>(optional:) merging the output produced by different runs of %theta</li>
 * <li>analyzing the output</li>
 *</ol>
 * All but the last point are well supported by %theta and explained below in more detail.
 *
 * \subsection model_def Model definition
 *
 * The first step to do in any analysis with %theta is to translate you model (like the one above) into a %theta
 * configuration.
 *
 * Now, have a look at the <tt>examples/gaussoverflat.cfg</tt> configuration file. The syntax is actually quite simple
 * but might require some time to get used to. For now, only two
 * recurring terms are introduced: "setting" is any statement of the form "parameter = value;" and
 * "setting group" which is a set of settings enclosed in curly braces, e.g., in
 * <pre>
 * mass = {
 *   range = (500.0, 1500.0);
 *   nbins = 200;
 * };
 * </pre>
 * the right hand side of the "mass" setting is a setting group containing a "range" setting (which has a list as type) and "nbins"
 * setting (an integer type). See the libconfig reference linked in the \ref ack section for a detailed description of the configuration file syntax.
 *
 * At the top of the configuration file, the parameters and observables you want to use are defined:
 * there is one observable "mass" with the range [500, 1500] and 200 bins. Note that %theta does
 * not care at all about units and that observables are <em>always</em> binned.
 * The parameters of this model are defined next: "s" is
 * the (poisson) mean number of signal events after your selection, "b" is the mean number of
 * background events. Their "default" values are used later for pseudo data generation. As we want to create test statistics for the
 * "background only" hypothesis, we set the signal parameter to zero. The range of these parameters
 * is mainly used as contraint for fits. You usually define it as large as physically makes sense. In this case,
 * the parameters can take any non-negative value.
 *
 * Next, the model "gaussoverflat" is defined, where the expectation for the "mass" observable is specified. As discussed above,
 * it is a linear combination of s signal events which are gaussian and b background events which are flat. This linear combination
 * of different components is expressed as different setting groups where you specify \c coefficients and \c histogram for each component.
 *
 * After the observable specification, there is a list of constraints. Constraints
 * specified in the model will be used
 * <ul>
 * <li>as additional term in the likelihood function (Bayesianically, these are priors)</li>
 * <li>If throwing pseudo experiments, they are used to choose the parameters' values for pseudo data creation (if no contraint is
 *      defined for a parameter, always its default value is used for pseudo data generation).
 * </ul>
 * The "constraint" settings group  concludes the definition of the model.
 *
 * \subsection conf_stat Configuration of the statistical methods to apply
 *
 * After having defined the model, the next thing to take care of is the statistical method you want to apply.
 *
 * In this case, this is done by the "hypotest" settings group.
 *
 * This settings group defines a statistical method (also called "producer", as it produces
 * data in the output %database) of type "deltanll_hypotest". This producer (which is documented \link deltanll_hypotest here \endlink)
 * expects two more settings: "signal-plus-background" and "background-only".
 * They are both setting groups which specify special parameter values to apply to
 * get these two model varaints from the original model. In this case, the "background-only" model is given
 * by the setting group "{s=0.0;}".For the "signal-plus-background", no constraints have to be applied,
 * therefore, an empty settings group is given: "{}".
 *
 * Whenever a producer modeule runs, it will be provided with a model and data. The deltanll_hypotest producer will
 * <ol>
 * <li> Construct the likelihood function of the model, given the data</li>
 * <li> Minimize the negative logarithm of the likelihood function with the "signal-plus-background" parameter values fixed</li>
 * <li> Minimize the negative log-likelihood function with the "background-only" parameter values fixed</li>
 * <li> save the two values of the negative-log-likelihood in the result table</li>
 * </ol>
 *
 * \subsection conf_run Configuration of the run
 *
 * Having configured the model and a statistical method, we have to glue them together. In this case, we want to make
 * many pseudo experiments in order to determine the distribution of the likelihood-ration test-statistics.
 *
 * This is done by the "main" settings group. It defines a \link plain_run plain run \endlink which
 * throws random pseudo data from a model and calls a list of producers. The results will be written
 * as SQL %database to a file with the path specified with "result-file".
 *
 * Pseudo data is thrown according to the configured model with following sequence:
 * <ol>
 *  <li>Determine a random value for each model parameter. This is done with a the constraint given in the model for that parameter, if it exists.
 *        Otherwise, the default value from the parameter definition is used.</li>
 *  <li>For each observable, use these parameters to evaluate the Histograms and coefficients.</li>
 *  <li>For each observable, add all components (i.e., coefficients and histograms)</li>
 *  <li>For each observable, draw a poisson-random sample from the obtained summed histogram</li>
 * </ol>
 *
 * \subsection running_theta Executing theta
 *
 * To actually do all the work, you have to call %theta with the configuration file name as argument
 * <pre>
 * bin/theta examples/gaussoverflat.cfg
 * </pre>
 *
 * \subsection analyzing Analyzing the output
 *
 * The output of a run of %theta is saved as SQLite %database to the output file configured in the run
 * settings group. %theta does not (at least so far) provide any tools to analyze this output. However
 * it is not hard to write your own program which goes through the result tables and makes some plots; see
 * \c root/histos.cxx for a starting point.
 *
 * In order to know which tables are in the file, have a look at the documentation of the theta::Run
 * object which is responsible to create some general-purpose tables. Furthermore, there is one table per producer
 * for which the table format is documented there.
 *
 * \section internal Overview of theta internals
 *
 * In the previous section you have seen a simple use case of %theta. Most of the components and concepts of
 * %theta have been touched there. To better understand the documentation, it is useful to know what happens "behind the scenes".
 *
 * First of all, you might have noticed that the configuration file format is <i>hierarchical</i> and consists of many
 * named setting groups. %theta has a very modular architecture which makes it easy to write extensions for; one important
 * thing to remember at this point is:<b>Any setting group containing a "type="&lt;typename&gt;";" setting is used to construct a C++ object of class &lt;typename&gt; via a plugin system.</b>
 *
 * This is very useful if you search for documentation: if you encounter a setting like type="deltanll_hypotest", you now know that you have
 * to search for the documentation at \link deltanll_hypotest \endlink.
 *
 * So far, plugins can be defined for following types:
 * <ul>
 * <li>\link theta::HistogramFunction HistogramFunction\endlink: used in the "histogram=..."-setting in the observables specification of a model</li>
 * <li>\link theta::Function Function\endlink: used as coefficients of the components of an observable specification in a model</li>
 * <li>\link theta::Minimizer Minimizer\endlink: used by some producers such as maximum likelihood, profile likelihood methods</li>
 * <li>\link theta::Run Run \endlink: the top-level object which invokes the pseudo data creation and producers</li>
 * <li>\link theta::Distribution Distribution\endlink: used in model constraints or as priors in a statistical method</li>
 * <li>\link theta::Producer Producer\endlink: statistical method called by a Run object</li>
 * </ul>
 *
 * To define and use your own plugin, you have to:
 *<ol>
 * <li>Define a new class derived from a class in the list above and implement all its pure virtual methods and a constructor
 *     taking a \link theta::plugin::Configuration Configuration \endlink object as the only argument.</li>
 * <li>In a .cpp-file, call the REGISTER_PLUGIN(yourclass) macro</li>
 * <li>Make sure to compile and link this definition to a shared-object file.</li>
 * <li>In the configuration file, make sure to load the shared-object file as plugin. You can now use the plugin defined as any other %theta component via
 *     a setting group containing type="yourclass";
 *</ol>
 * For all these cases, you can have a look at the \c plugins/ directory, which contains the core plugins of %theta.
 *
 * \section plugins Available Plugins
 *
 * Core and root plugins, by type:
 * <ul>
 * <li>\link theta::HistogramFunction HistogramFunction\endlink: used in the "histogram=..."-setting in the observables specification of a model:
 *     <ul>
 *         <li>fixed_gauss for defining a normal distribution with fixed (i.e., not parameter dependent) mean and standard deviation</li>
 *         <li>fixed_poly for a polynomial of arbitrary order with fixed (i.e., not parameter dependent) coefficients</li>
 *         <li>interpolating_histo to interpolate between one "nominal" and several "distorted" histograms for the generic treatment of systematic uncertainties</li>
 *         <li>root_histogram to read a histogram from a root file</li>
 *     </ul>
 *   </li>
 * <li>\link theta::Function Function\endlink: used as coefficients of the components of an observable specification in a model. Currently, no core plugins are available.</li>
 * <li>\link theta::Minimizer Minimizer\endlink: used by some producers such as maximum likelihood, profile likelihood methods:
 *     <ul>
 *       <li>root_minuit using MINUIT via ROOT</li>
 *       <li>(not yet implemented) lbfgs using liblbfgs</li>
 *     </ul>
 * </li>
 * <li>\link theta::Run Run\endlink: the top-level object which invokes the pseudo data creation and producers:
 *    <ul>
 *       <li>plain_run throwing pseudo data and calling all producers</li>
 *       <li>scan_run scanning through a given model parameter. For each fixed parameter value, throw pseudo data and call the producers</li>
 *       <li>(not yet implemented:) data_run apply the list of statistical methods to data</li>
 *    </ul>
 * </li>
 * <li>\link theta::Distribution Distribution\endlink: used in model constraints or as priors in a statistical method:
 *    <ul>
 *       <li>gauss normal distribution in one or more dimensions, including arbitrary correlations</li>
 *       <li>log_normal log-normal distribution in one dimension</li>
 *     </ul>
 * </li>
 * <li>\link theta::Producer Producer\endlink: statistical method called by a Run object</li>
 *    <ul>
 *       <li>deltanll_hypotest creates likelihood-ratio test statistics to find the critical region for rejecting a "background only" null hypothesis</li>
 *       <li>(not yet implemented:) deltanll_intervals interval creation based on the difference in the negative-log-likelihood function</li>
 *       <li>mle maximum likelihood estimator estimates parameter values and errors using a minimizer on the negative-log-likelihood function</li>
 *       <li>(not yet implemented:) mcmc_quantiles Quantile estimator based on Markov-Chain Monte-Carlo to be used for interval estimation</li>
 *       <li>(not yet implemented:) mcmc_marginal Determine the marginal distribution (as Histogram) for a parameter</li>
 *       <li>mcmc_posterior_ratio analogue of deltanll_hypotest, but integrates over any free parameters instead of minimizing</li>
 *       <li>pseudodata_writer writes out the created pseudo data. Not really a statistical method, but technically implemented as a Producer as well</li>
 *    </ul>
 * </ul>
 */
 
 
 /** \page cmd_interface Command line interface
  *
  * %theta comes with two programs meant to be run from the command line: %theta and merge.
  *
  * \section cmd_theta theta
  *
  * The %theta command expects one or two arguments: the configuration file and (optionally) the name of the run setting
  * in this file. If the run name is not given, "main" is assumed as name for the run.
  *
  * %theta parses the configuration file, constructs the \link theta::Run Run \endlink object from the specified setting group
  * and invoke its \link theta::Run::run run method \endlink.
  *
  * %theta will output the current progress if running on a tty, unless disabled via the \c -q (or \c --quiet) option.
  *
  * If you send the \c SIGINT signal to %theta (e.g., by hitting ctrl+C on a terminal running %theta),
  * it will exit gracefully as soon as possible (which usually means
  * after the run has called all producers on the current pseudo data). This feature is useful for
  * interactive use if the whole run takes too long but you still want to be able to
  * analyze the output produced so far. It is also useful part of a batch job script
  * which can send this signal just before the job reaches
  * the maximum time by which it is killed by the batch system.
  *
  * \section cmd_merge merge
  *
  * The \c merge program is used to merge result databases from different runs of the %theta command into a single one.
  * The \c merge program will only work on <em>compatible</em> databases, i.e., databases which contain the same tables
  * and where tables contain the same columns. Usually, you should only attempt to merge result databases which were produced
  * using the same configuration file.
  *
  * \c merge re-maps the runid entry in the input files such that the output file contains unique runid entries.
  *
  * \c merge will refuse to merge result databases which use the same random number generator seed.
  *
  * A typical invocation is :
  * <pre>
  * merge --outfile=merged.db result1.db result2.db result3.db
  * </pre>
  * which merges the files result{1,2,3}.db into the output file merged.db. If the output file exists,
  * it will be overwritten.
  *
  * Another possibility is to merge all files matching \c *.db within one directory via
  * <pre>
  * merge --outfile=merged.db --in-dir=results
  * </pre>
  *
  * The only other supported option is \c -v or \c --verbose which increases the verbosity of merge.
  */
 
/** \page design Design Goals of Theta
 *
 * I'm not a friend of some general statements of intention one can write about the meta-goals of a program. Still, I do raise some points here
 * because it will make some design choices clearer and because I truly believe in these principles and actually affect
 * the day-to-day work with the code, as they make me re-think all design choices I make (even after having implemented them).
 *
 * The main design goals of %theta are:
 * <ul>
 * <li>correctness</li>
 * <li>simplicity</li>
 * <li>performance</li>
 * <li>ease of use</li>
 * <li>extensibility</li>
 * <li>use best practices</li>
 * </ul>
 *
 * %theta is <b>not</b> designed to implement every method you can think because this would contradict
 * the "simplicity" and "ease of use" almost inevitably and would also make it much harder to achieve
 * "correctness" and "performance".
 *
 * <b>Correctness</b> comes first. It means that (i) methods should work as documented (and, of course, be documented). (ii) If anything at all
 * prevents the method from performing correctly, it should fail as soon as possible and not produce wrong result or cause a failure much later.
 * (iii) Each unit (class, method) should have a unit test which tests its functionality, including failure behaviour.
 *
 * <b>Simplicity</b> implies that %theta has a limited scope of application. It is not intended to be the tool for every problem you
 * can think of. As consequence, (i) there have been some fundamental design choices, e.g., only to support binned
 *  representations of pdfs and data (which, for many applications, is not a problem at all as the bin size can be made arbitrarily small),
 * (ii) any class and method has one simple, well-defined task only. This implies that classes
 *  generally have only very few public methods and that classes and methods are relatively easy to
 *  document (and very lengthy documentation is generally a sign of too much functionality pressed into one class or method).
 *
 * <b>Performance</b> has to stand back of correctness and simplicity. However, many code changes increasing performance
 * do not need a re-design of a class' public interface at all. All changes which <i>provably</i>(!) increase performance should actually be done.
 * An example of a trade-off in favour of simplicity was a re-design of the random number generator interface: if one defines
 * all functions and methods which require a random number generator as argument as template functions with the random number generator type as template
 * parameter, one can gain up to 50% performance compared to a system using an \link theta::RandomSource abstract class \endlink implementing
 * \link theta::RandomSourceTaus concrete \endlink random number \link theta::RandomSourceMersenneTwister generators \endlink
 * by implementing this interface as the latter requires additional virtual function table lookups.
 *
 * <b>Ease of use</b> means good documentation and examples. It is also closely
 * related to simplicity: simple classes are easy to document and easier to understand as they have
 * a clear and narrowly scoped functionality. There is also less to document
 * if the classes are not bloated. <em>ease of use</em> also means that it should be easy to
 * use the program/library correctly and hard to use it incorrectly. In particular, wherever possible,
 * constructs which obviously do not make sense should fail to compile or generate a runtime error
 * as soon as possible.
 *
 * <b>Extensibility</b>  means that it should be easy to re-use parts of the application and have totally different definitions of other parts.
 * For example, you might want to use the Markov-Chain Monte-Carlo functions with a completely different likelihood / posterior. Or you want to use the same
 * likelihood and statistical methods, but with a completely different model definition. This is possible in %theta via the use of a
 * <i>plugin system</i>, in which plugins are just shared objects files you write loaded at runtime. Plugins integrate seamlessly in the program in that
 * you can configure them in the exact same way as the "native" parts of %theta.
 *
 * <b>Best practices</b> beyond the points already mentioned include (mainly taken from python's zen):
 * (i) explicit is better than implicit: avoid "magic names" or "magic numbers" which trigger special behaviour or which have special meaning.
 * (ii) There should never be a need to guess how to code something, i.e., there should be as little ambiguities left of how to write working code
 * or a working config file after reading the documentation. (iii) There should be one (and preferrably only one) obvious way of getting
 * something done.
 */

/** \brief Common namespace for %theta
 */
namespace theta{}
