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
 *   <li>\subpage installation Installation explains how to obtain and compile %theta</li>
 *   <li>\subpage intro Introduction describes how to run %theta; a first example is discussed and an introduction to the internals of %theta.
 *        In also contains a \ref plugins "list of available plugins".<li>
 *   <li>\subpage design "Design Goals of theta" contains some thoughts about what the code of %theta should be like.
 *       You should read that either if you want to contribute code to %theta or if you want to know what makes %theta
 *       different to other software you often deal with in high-energy physics.</li>
 * </ol>
 *
 * \section ack Acknowledgement
 *
 * I would like to thank the authors of the excellent software packages used by %theta:
 * <ul>
 * <li>\link http://www.hyperrealm.com/libconfig/libconfig.html libconfig \endlink A
 *      well-written, well-documented C/C++ library for processing configuration files with a very simple and elegant API.</li>
 * <li>\link http://www.chokkan.org/software/liblbfgs/ liblbfgs \endlink An implementation of
 *     the Limited-memory Broyden-Fletcher-Goldfarb-Shanno minimization algorithm</li>
 * </ul>
 * These libraries are included in the distribution of %theta.
 *
 * Furthermore, some parts of numerical algorithms have been copied from the excellent \link http://www.gnu.org/software/gsl/ GNU Scientific Library (GSL) \endlink.
 *
 * Last but not least, I want to thank Jasmin Gruschke who tested %theta from an end-user point of view, made useful
 * suggestions and bravely endured backward-incompatible changes.
 *
 * \section license License
 *
 * %theta is licensed under the \link http://www.gnu.org/copyleft/gpl.html GPL \endlink.
 *
 * \section ref References
 * The %theta make use of:
 * <ol>
 *   <li><em>Matsumoto, Makoto and Nishimura, Takuji:</em> \link http://doi.acm.org/10.1145/272991.272995 "Mersenne twister: a 623-dimensionally equidistributed uniform pseudo-random number generator"\endlink,
 *        ACM Trans. Model. Comput. Simul. 1, 1998</li>
 *   <li><em>Pierre L'Ecuyer:</em> "Maximally Equidistributed Combined Tausworthe Generators", Math. Comp. 65, 1996</li>
 *   <li><em>Pierre L'Ecuyer:</em> "Tables of Maximally Equidistributed Combined LFSR Generators", Math. Comp. 68, 1999</li>
 *   <li><em>George Marsaglia and Wai Wan Tsang:</em> "The Ziggurat Method for Generating Random Variables", Journal of Statistical Software 8, 2000</li>
 *   <li><em>A. Gelman, G. O. Roberts, and W. R. Gilks:</em> "Efficient Metropolis Jumping Rules", Bayesian Statistics 5, 1996</li>
 *   <li><em>TODO:</em> quantile estimation ...</li>
 * </ol>
 */


/** \page installation Installation
 *
 * \section obtaining Obtaining theta
 *
 * %theta is available as source-code distribution via subversion only. The latest version can be obtained by running
 * <pre>
 * svn co https://ekptrac.physik.uni-karlsruhe.de/svn/theta/trunk theta
 * </pre>
 * If you want to sompile it within CMSSW (which provides the dependencies), make sure to
 * check it out in a CMSSW area.
 *
 * \section building Building theta
 *
 * \subsection with_cmssw With CMSSW
 *
 * Copy the source tree to an initialized \c CMSSW directory structure and run
 * \c make. In this case, boost and sqlite3 which are shipped with CMSSW will be used.
 * The main executable will be located in
 * <pre>
 * bin/theta
 * </pre>
 *
 * Before running it, execute
 * <pre>
 * source setenv.sh
 * </pre>
 * from the installation root to adapt \c LD_LIBRARY_PATH for the shared objects of boost and sqlite3. This ensures
 * that the ones from the CMSSW distribution will be used.
 *
 * If you want to test that everything is Ok, you can run the unit-tests provided with %theta, by running
 * <pre>
 * make run-test
 * </pre>
 *
 * In summary, a complete set of commands to check out, compile, test and run %theta with cmssw
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
 * <li>Boost</li>
 * <li>sqlite3</li>
 * </ol>
 * There are packages available for these on many distribution.
 *
 * Then, you can follow the instructions above:
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
 *
 * %Theta is about modeling and statistical inference. For %theta, "model" means
 * a specification of the probability density of one or more observables as function of
 * some parameters, including possible probability densities of the parameters. To make this
 * more clear, a concrete example is discussed first where you get an overview over how %theta works
 * from the point of view of a user.
 *
 * In the second section, some internals of %theta are explained which are good to know even if you
 * do not plan to extend %theta.
 *
 * \section first_example First example
 *
 * Suppose you search for a new particle. After a sophsticated event selection,
 * you have events containing candidates of your new particle.
 * For each of these events, you can reconstruct the mass. From your Monte-Carlo simulation,
 * you conclude that your signal has a distribution in this reconstructed mass in the
 * form of a gaussian with mean 1000 GeV/c^2 and width 250 GeV/c^2, whereas your background
 * is expected to be flat in the region from 500 to 1500 GeV/c^2 which should be used to further constrain
 * your background.
 *
 * You do the studies mainly at one fixed integrated luminosity L. From a background fit
 * to a sideband you expect that you can constrain your background poisson mean in the signal
 * region to 1600 +- 200 events. The model of your signal allows for a large variety of signal
 * cross sections; the standard model predicts predicts no "signal".
 *
 * In this analysis, there are many questions frequently asked. Some of them are:
 * <ol>
 * <li>At some fixed integrated luminosity, and assuming that the SM is true, what is your expected upper limit
 *  on the signal cross section?</li>
 * <li>Given the actually measured data, what is your upper limit?</li>
 * <li>Assume that your signal has cross section such that in the mean case 200 signal events pass your event selection.
 *    Can you exclude the standard model null-hypothesis at 3 (5) sigma in this case?
 * </ol>
 *
 * For this introduction, we consider the creation of likelihood-ratio test statistics for the
 * "background only" null hypothesis in order to address the last question.
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
 *All but the last point are well supported by %theta and explained below in more detail.
 *
 * \subsection model_def Model definition
 *
 * The first step to do in any analysis with %theta is to translate you model (like the one above) into a %theta
 * configuration.
 *
 * Now, have a look at the <tt>examples/gaussoverflat.cfg</tt> configuration file. For now, only two
 * recurring terms are introduced: "setting" is any statement of the form "parameter = value;" and
 * "setting group" which is a set of settings enclosed in curly braces, e.g., in
 * <pre>
 * mass = {
 *   range = (500.0, 1500.0);
 *   nbins = 200;
 * };
 * </pre>
 * the right hand side of the "mass" setting is a setting group containing a "range" setting (which has a list as type) and "nbins"
 * setting (an integer type). See \link config link below \endlink for a detailed description of the configuration file syntax.
 *
 * At the top of the configuration file, the parameters and observables you want to use are defined:
 * there is one observable "mass" with the range [500, 1500] and 200 bins. Note that theta does
 * not care at all about units and that observables are <em>always</em> binned.
 * The parameter this model depends on are defined next: "s" is
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
 *        Otherwise, the default value from the parameter definition is used.</ul>
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
 * it is not hard to write you own program which goes through the result tables and makes some plots; see
 * \c root/histos.cxx for a starting point.
 *
 * In order to know which tables are in the file, have a look at the documentation of the \link theta::RunT Run\endlink
 * object which is responsible to create some general-purpose tables. Furthermore, there is one table per producer
 * for which the table format is documented there.
 *
 * \section internal Overview of theta internals
 *
 * In the previous section you have seen a simple use case of theta. Most of the components and concepts of
 * theta have been touched there. To better understand the documentation, it is useful to know what happens "behind the scenes".
 *
 * First of all, you might have noticed that the configuration file format is <i>hierarchical</i> and consists of many
 * named setting groups. %theta has a very modular architecture which makes it easy to write extensions for; one important
 * thing to remember at this point is:
 * \attention
 *  Any setting group containing a "type="&lt;typaname&gt;";" setting is used to construct a C++ object of class &lt;typename&gt; via a plugin system.
 *
 * This is very useful if you search for documentation: if you encounter a setting like "type="deltanll_producer"", you now know that you have
 * to search for the documentation at \link deltanll_producer \endlink.
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
 * <li>Define a new class derived from one classes in the list above and implement all its pure virtual methods and a constructor
 *     taking a \link theta::plugin::Configuration & Configuration \endlink object as the only argument.</li>
 * <li>In a .cpp-file, call the REGISTER_PLUGIN(yourclass) macro</li>
 * <li>Make sure to compile and link this definition to a shared-object file.</li>
 * <li>In the configuration file, make sure to load the shared-object file as plugin. You can now use the plugin defined as any other %theta component via
 *     a setting group containing type="yourclass";
 *</ol>
 * For all these cases, you can have a look at the \c plugins/ directory, which contains the core plugins of %theta. E.g., a
 * complete, more complex example is given by plugins/interpolating-histogram.{cpp,hpp} which defines Histogram interpolation
 * via additional model parameters, meant for a generic treatment of systematic uncertainties. You will find that, given the complexity of the problem,
 * the required code is manageable.
 *
 * \section plugins Plugins
 *
 * Core and root plugins, by type:
 * <ul>
 * <li>\link theta::HistogramFunction HistogramFunction\endlink: used in the "histogram=..."-setting in the observables specification of a model:
 *     <ul>
 *         <li>\link fixed_gauss <b>fixed_gauss</b>\endlink for defining a normal distribution with fixed (i.e., not parameter dependent) mean and standard deviation</li>
 *         <li>\link fixed_poly <b>fixed_poly</b>\endlink for a polynomial of arbitrary order with fixed (i.e., not parameter dependent) coefficients</li>
 *         <li>\link interpolating_histo <b>interpolating_histo</b>\endlink to interpolate between one "nominal" and several "distorted" histograms for the generic treatment of systematic uncertainties</li>
 *         <li>\link root_histogram <b>root_histogram</b>\endlink to read a histogram from a root file</li>
 *     </ul>
 *   </li>
 * <li>\link theta::Function Function\endlink: used as coefficients of the components of an observable specification in a model. Currently, no core plugins are available.</li>
 * <li>\link theta::Minimizer Minimizer\endlink: used by some producers such as maximum likelihood, profile likelihood methods:
 *     <ul>
 *       <li>\link root_minuit <b>root_minuit</b>\endlink using MINUIT via ROOT</li>
 *       <li>(not yet implemented) \link lbfgs <b>lbfgs</b>\endlink using liblbfgs</li>
 *     </ul>
 * </li>
 * <li>\link theta::Run Run \endlink: the top-level object which invokes the pseudo data creation and producers:
 *    <ul>
 *       <li>\link plain_run <b>plain_run</b> \endlink throwing pseudo data and calling all producers</li>
 *       <li>\link scan_run <b>scan_run</b> \endlink scanning through a given model parameter. For each fixed parameter value, throw pseudo data and call the producers</li>
 *       <li>(not yet implemented:) \link data_run <b>data_run</b> \endlink apply the list of statistical methods to data</li>
 *    </ul>
 * </li>
 * <li>\link theta::Distribution Distribution\endlink: used in model constraints or as priors in a statistical method:
 *    <ul>
 *       <li>\link gauss <b>gauss</b>\endlink normal distribution in one or more dimensions, including arbitrary correlations</li>
 *       <li>\link lognormal <b>lognormal</b>\endlink log-normal distribution in one dimension</li>
 *     </ul>
 * </li>
 * <li>\link theta::Producer Producer\endlink: statistical method called by a Run object</li>
 *    <ul>
 *       <li>\link deltanll_hypotest <b>deltanll_hypotest</b>\endlink creates likelihood-ratio test statistics to find the critical region for rejecting a "background only" null hypothesis</li>
 *       <li>(not yet implemented:) \link deltanll_intervals <b>deltanll_intervals</b>\endlink interval creation based on the difference in the negative-log-likelihood function</li>
 *       <li>\link mle <b>mle</b>\endlink maximum likelihood estimator estimates parameter values and errors using a minimizer on the negative-log-likelihood function</li>
 *       <li>(not yet implemented:)\link mcmc_quantiles <b>mcmc_quantiles</b>\endlink Quantile estimator based on Markov-Chain Monte-Carlo to be used for interval estimation</li>
 *       <li>(not yet implemented:)\link mcmc_marginal <b>mcmc_marginal</b>\endlink Determine the marginal distribution (as Histogram) for a parameter</li>
 *       <li>(not yet implemented:)\link mcmc_posterior_ratio <b>mcmc_posterior_ratio</b>\endlink analogue of deltanll_hypotest, but integrates over any free parameters instead of minimizing</li>
 *       <li>\link pseudodata_writer <b>pseudodata_writer</b>\endlink writes out the created pseudo data. Not really a statistical method, but technically implemented as a Producer as well</li>
 *    </ul>
 * </ul>
 */
 
 
 /*
 * A typical execution of the <tt>%theta</tt> main program consists of:
 * <ol>
 * <li>Read in and parse the config file supplied as command-line argument</li>
 * <li>Use the "parameters" and "observables" blocks to save the information supplied there in a \link theta::VarIdManager VarIdManager \endlink
 *     instance. This object saves information about parameters and observables like the ones supplied in the configuration file. </li>
 * <li>Locate the "main" settings block (or the one supplied on the command line) and use it to
 *     create an instance of the configured \link theta::RunT Run \endlink object.
 *     The creation of the this object, in turn, will invoke:
 *   <ol>
 *     <li>Create the model(s). The model will create some histograms defined
 *        in the model (as these histograms depend on model parameters, they are called HistogramFunctions).
 *        The constraints definition in a model will lead to the creation of Distributions and
 *        the coefficients of the HistogramFunction will be created in for of Functions.
 *     </li>
 *   <li>Create an instance of each of the configured producers. This might involve creating
 *    a minimizer, if the producer depends on minimization of the negative log-likelihood.</li>
 *   <li>Create the pseudo-random number generator</li>
 *   <li>Create the output database</li>
 *   </ol>
 * <li>Execute the <tt>Run</tt>. What execution means depends on the configured type of the Run. Typically,
 *   it will throw pseudo experiments and, for each pseudo experiment, apply some statistical methods ("producers").</li>
 * </ol>
 *
 * \section resultfile Result File
 *
 * The result file is a SQLite %database file. It always contains some tables created by the \link theta::RunT Run \endlink object
 * which are documented there. Note that derived classes of %Run can create additional tables.
 *
 * Additionally, one table per producer is created. The table names match the producer names, as given in the configuration file. The
 * table format is producer-specific and documented seperately for each producer.
 *
 * For example, the "hypotest" producer in the gaussoverflat example will write its results into a table called "hypotest"
 * with a format documented in DeltaNLLHypotestTable.
 *
 * \section config Configuration file format
 *
 * %Theta uses \link http://www.hyperrealm.com/libconfig/ libconfig \endlink as configuration file
 * format. The format is described in detail
 * \link http://www.hyperrealm.com/libconfig/libconfig_manual.html#Configuration-Files here \endlink.
 */
 
 
/* \page arch Architecture of %theta
 *
 * A large part of theta is already thouroughly documented. However, to use the
 * documentation effectively, some knowledge of the architecture of %theta is
 * essential.
 *
 * While the original goal of %theta was to implement some statistical methods and
 * utilities for it (e.g., make it easy to define a model, create the likelihood function for the model, given data,
 * run a large number of pseudo experiments with some statistical methods, compare different methods to each other, etc.),
 * %theta has developed to a <i>framework</i> in the sense that it provides interfaces
 * (in the form of abstract C++ classes) for new functionality.
 *
 * A typical execution of the <tt>%theta</tt> main program consists of:
 * <ol>
 * <li>Read in and parse the config file supplied as command-line argument</li>
 * <li>Create an instance of the configured <tt>Run</tt> object. This, in turn, will invoke:
 *   <ol>
 *     <li>Create the model(s). The model will create some histograms defined
 *        in the model (as these histograms depend on model parameters, they are called HistogramFunctions).
 *        The constraints definition in a model will lead to the creation of Distributions and
 *        the coefficients of the HistogramFunction will be created in for of Functions.
 *     </li>
 *   <li>Create an instance of each of the configured producers. This might involve creating
 *    a minimizer, if the producer depends on minimization of the negative log-likelihood.</li>
 *   <li>Create the pseudo-random number generator</li>
 *   <li>Create the output database</li>
 *   </ol>
 * <li>Execute the <tt>Run</tt>. What execution means depends on the configured type of the Run. Typically,
 *   it will throw pseudo experiments and, for each pseudo experiment, apply some statistical methods ("producers").</li>
 * </ol>
 *
 * Now, everywhere in the above list where the word "create" was used, you might want
 * to extent %theta and provide your own version of what follows this word "create".
 */

/** \page design Design Goals of Theta
 *
 * I'm not a friend of some general statements of intention one can write about the meta-goals of a program. Still, I do raise some points here
 * because it will make some design choices clearer and because I truly believe in these principles and actually affect
 * the day-to-day work with the code, as they make me re-think all design choices I make (even after having implemented them).
 *
 * The main design goals of theta are:
 * <ul>
 * <li>correctness</li>
 * <li>simplicity</li>
 * <li>performance</li>
 * <li>ease of use</li>
 * <li>extensibility</li>
 * <li>use best practices</li>
 * </ul>
 *
 * Theta is <b>not</b> designed to implement every method you can think because this would contradict
 * the "simplicity" and "ease of use" almost inevitably and would also make it much harder to achieve
 * "correctness" and "performance".
 *
 * <b>Correctness</b> comes first. It means that (i) methods should work as documented (and, of course, be documented). (ii) If anything at all
 * prevents the method from performing correctly, it should fail as soon as possible and not produce wrong result or cause a failure much later.
 * (iii) Each unit (class, method) should have a unit test which tests its functionality, including failure behaviour.
 *
 * <b>Simplicity</b> implies that theta has a limited scope of application. It is not intended to be the tool for every problem you
 * can think of. As consequence, (i) there have been some fundamental design choices, e.g., only to support binned representations of pdfs and data. Usually
 * this is not a problem as you can always choose a fine binning, but there will never be support for unbinned pdfs or data.
 * (ii) any class and method has one simple, well-defined task only. This implies that classes generally have only very few public methods an that classes
 * and methods are relatively easy to document (if (required) documentation is lengthy, it is generally a sign of too much functionality pressed into
 * a class / method).
 *
 * <b>Performance</b> has to stand back of correctness and simplicity. However, many code changes increasing performance
 * do not need a re-design of a class' public interface at all. All changes which <i>provably</i>(!) increase performance should actually be done.
 *
 * <b>ease of use</b> means good documentation and examples. It is also closely
 * related to simplicity: simple classes are easy to document and easier to understand as they have
 * a clear and narrowly scoped functionality. There is also less to document
 * if the classes are not bloated. <em>ease of use</em> also means that it should be easy to
 * use the program/library correctly and hard to use it incorrectly. In particular, wherever possible,
 * constructs which obviously do not make sense should fail to compile or generate a runtime error
 * as soon as possible.
 *
 * <b>Extensibility</b>  means that it should be easy to re-use parts of the application and have totally different definitions of other parts.
 * For example, you might want to use the Markov-Chain Monte-Carlo functions with a completely different likelihood / posterior. Or you want to use the same
 * likelihood and statistical methods, but with a completely different model definition. This is possible in theta via the use of a
 * <i>plugin system</i>, in which plugins are just shared objects files you write loaded at runtime. Plugins integrate seamlessly in the program in that
 * you can configure them in the exact same way as the "native" parts of theta.
 *
 * <b>Best practices</b> beyond the points already mentioned include (mainly taken from python's zen):
 * (i) explicit is better than implicit: avoid "magic names" or "magic numbers" which trigger special behaviour or which have special meaning.
 * (ii) There should never be a need to guess how to code something, i.e., there should be as little ambiguities left of how to write working code
 * or a working config file after reading the documentation. (iii) There should be one (and preferrably only one) obvious way of getting
 * something done.
 */

/** \brief Common namespace for almost all classes of %theta.
 */
namespace theta{}

