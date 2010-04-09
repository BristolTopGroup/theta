/* this file is not for inclusion in C++ code. It is only here
 * to provide a "mainpage" block for doxygen (and I didn't want to spoil some random
 * header with it ...). It also includes documentation of the "theta" namespace
 * which is distributed over many header files.
 */

/** \mainpage
 *
 *
 *  \image html theta-medium.png
 *
 * %theta is a framework for template-based statistical modeling and inference, focussing on problems
 * in high-energy physics. It provides the possibility for the user to express a "model", i.e.,
 * the expected data distribution, as function of physical parameters. This model can be used
 * to make statistical inference about the physical parameter of interest. Modeling is "template-based"
 * in the sense that the expected data distribution is always expressed as a sum of templates (which, in general,
 * depend on the model parameters).
 *
 * %theta supports a physicist doing data analysis to answer commonly arising statistical questions
 * such as large-scale pseudo-experiments for coverage tests, luminosity scans, definition of the critical region
 * for a hypothesis test, etc.
 *
 * The intention of %theta is to <em>support</em> the user in the sense that it provides the necessary tools
 * and documents to address many questions.
 * However, %theta does not intent to be an all-in-one device which suits every purpose. For example,
 * it is restricted to template-based modeling in the sense described above. Also, %theta does no sort of
 * plotting whatsoever.
 *
 * The documentation is split into several pages. If you are new to %theta, read them in this order:
 * <ol>
 *   <li>\subpage whatistheta "What theta can do" explains which kind of questions can (or cannot) be addressed with the help of %theta</li>
 *   <li>\subpage installation Installation explains how to obtain and compile %theta</li>
 *   <li>\subpage intro Introduction describes how to run %theta; a first example is discussed and
 *        an introduction to the internals of %theta. In also contains a \ref plugins "list of available plugins".</li>
 *   <li>\subpage cmd_interface "Command line interface" described the command line tools of %theta, namely the \c theta
 *     program and the \c merge program.</li>
 *   <li>\subpage extend "Extending theta" describes how to extend %theta using the plugin system</li>
 * </ol>
 *
 * Bug tracking and feature requests are managed in the <a href="https://ekptrac.physik.uni-karlsruhe.de/trac/theta">theta trac</a>.
 * If you find a bug or miss an essential feature, you can open a ticket there.
 *
 * Some additional information not necessarily of interest for every user:
 * <ol>
 *   <li>\subpage design "Design Goals of theta" contains some thoughts about what the code of %theta should be like.
 *       You should read that either if you want to contribute code to %theta or if you want to know what makes %theta
 *       different to other software you often deal with in high-energy physics.</li>
 *   <li>\subpage testing "Testing" describes how %theta is tested, i.e., unit tests and simple statistical cases
 *       for which the solution is known analytically (or can be otherwise be found independently).</li>
 * </ol>
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
 * %theta logos are available here as <a href="../logos/theta.pdf">pdf</a>, <a href="../logos/theta.eps">eps</a> and
 * <a href="../logos/theta.png">png</a>.
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
 *  <li>Template uncertainties can be treated effectively by fluctuating the templates before throwing the pseudo
 *    data from the tempaltes. This can be bin-by-bin independent errors or (if the user provides own plugins) any other errors.</li>
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
 * Combining models is an easy task: just extend the parameter and observable list and extend the model appropriately.
 *
 * \section whatcant What theta cannot do
 *
 * While %theta was written with a rather general use-case in mind, it is certainly not suited for all tasks, nor does it
 * do more than the core of the work. Specifically,
 * <ul>
 *  <li>%theta cannot plot. While it is not hard to get plots from the result database %theta produces, %theta itself
 *    has no plotting routines.</li>
 *  <li>%theta is not suited for cases where you have more than one observable per event, i.e., if to do multi-dimensional
 *     fits. In principle, handling multidimensional fits can be implmented in %theta via user-supplied plugins. However, it is
 *     not foreseen to suport this natively.</li>
 *  <li>%theta only supports binned distributions. While binning can
 *     also be chosen very fine, it is not foreseen to support trule 
 *     continuos distributions.</li>
 *  <li>%theta is mainly useful if only one parameter is of interest (i.e., if only one parameter in
 *      the model is not a nuisance parameter).
 *      For example, it is not implemented (or foreseen) to have confidence regions in a two-dimensional plane. This is not an
 *      architectural limitation of %theta but a limitation mainly of the current implementation of statistical
 *      methods. You are free to implement such a method as %theta plugin, using %theta as framework, though.</li>
 * </ul>
 *
 * These restrictions are intentional: by doing only a small number of tasks, these can be done efficiently and correctness
 * is easier to achieve. Also, %theta is easier to document and to understand if not bloated by additional code.
 * See \ref design for more information about this point.
 *
 * \section singletop Single Top Search
 *
 * 
 *
 */

 /* \section boostedtop Z' search
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
 * svn co https://ekptrac.physik.uni-karlsruhe.de/svn/theta/tags/april-2010 theta
 * </pre>
 * If you want to compile it within CMSSW (which provides the dependencies), make sure to check it
 * out in the \c src directory of a CMSSW directory.
 *
 * \section building Building theta
 *
 * \subsection with_cmake With cmake
 *
 * The recommended way to compile %theta is to use cross-platform make, <a href="http://www.cmake.org/">CMake</a>.
 * After getting the dependencies (sqlite3, boost and root), issue
 * <pre>
 *  cd theta #or where ever it was checked out
 *  mkdir build
 *  cmake ..
 *  make
 * </pre>
 *
 * It is also possible to compile %theta using Makefiles. However, note that these are simple, hand-written Makefiles
 * which do not include all dependencies correctly. Therefore, be sure to always issue
 * \code make clean \endcode after an update.
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
 * <li><a href="http://boost.org/">Boost</a>, a bundle of general-purpose C++ libraries, which is
 *    used in %theta for filesystem interface, option parsing, memory management through
 *    smart pointers, and multi-threading.</li>
 * <li><a href="http://sqlite.org/">sqlite3</a>, a light-weight, SQL-based %database engine which
 *    is used to store the results in %theta</li>
 * </ol>
 * There are packages available for these on many distribution.
 *
 * Then, you can follow the instructions above, skipping the CMSSW part:
 * <pre>
 *  svn co https://ekptrac.physik.uni-karlsruhe.de/svn/theta/trunk theta
 *  cd theta
 *  make
 *  source setenv.sh
 *  make run-test
 *  bin/theta examples/gaussoverflat.cfg
 * </pre>
 * If you install the dependencies in an unusual location (i.e., if headers and libraries
 * are not found automatically) you have to edit the SQLITE_INCLUDE, SQLITE_LIBS,
 * BOOST_INCLUDE, and BOOST_LIBS variables on top of \c Makefile.rules.
 */

/**
 * \page intro Introduction
 *
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
 * the right hand side of the "mass" setting is a setting group containing a
 * "range" setting (which has a list as type) and "nbins"
 * setting (an integer type). See the libconfig reference linked in the \ref ack section
 * for a detailed description of the configuration file syntax. Note that any right hand side
 * where a setting group is expected, it is also allowed to write a string of the form "@&lt;path&gt;"
 * where &lt;path&gt; is the path in the configuration file to use at this place instead.
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
 * Whenever a producer modeule runs, it will be provided with a model and data. The \ref deltanll_hypotest producer will
 * <ol>
 * <li> Construct the likelihood function of the model, given the data</li>
 * <li> Minimize the negative logarithm of the likelihood function with the "signal-plus-background" parameter values fixed</li>
 * <li> Minimize the negative log-likelihood function with the "background-only" parameter values fixed</li>
 * <li> save the two values of the negative-log-likelihood in the result table</li>
 * </ol>
 * For more details, refer to the documentation of \ref deltanll_hypotest.
 *
 * \subsection conf_run Configuration of the run
 *
 * Having configured the model and a statistical method, we have to glue them together. In this case, we want to make
 * many pseudo experiments in order to determine the distribution of the likelihood-ration test-statistics.
 *
 * This is done by the "main" settings group. It defines a \ref plain_run which
 * throws random pseudo data from a model and calls a list of producers. The results will be written
 * as SQL %database to a file with the path specified with "result-file".
 *
 * Pseudo data is thrown according to the configured model with following sequence:
 * <ol>
 *  <li>Determine a random value for each model parameter. This is done with a the constraint given
 *        in the model for that parameter, if it exists.
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
 * \section plugins Available Plugins
 *
 * Everywhere in the configuration file where there is a "type=..." setting, the plugin system is used
 * to construct an object the corresponding type. The string given in the "type=..." setting is the C++ class
 * name used. The configuration is documented at the class documentation.
 *
 * Core and root plugins, by type:
 * <ul>
 * <li>\link theta::HistogramFunction HistogramFunction\endlink: used in the "histogram=..."-setting in the observables
 *      specification of a model:
 *     <ul>
 *         <li>fixed_gauss for defining a normal distribution with fixed (i.e., not parameter dependent) mean and standard deviation</li>
 *         <li>fixed_poly for a polynomial of arbitrary order with fixed (i.e., not parameter dependent) coefficients</li>
 *         <li>interpolating_histo to interpolate between one "nominal" and several "distorted" histograms for the
 *             generic treatment of systematic uncertainties</li>
 *         <li>root_histogram to read a histogram from a root file</li>
 *     </ul>
 *   </li>
 * <li>\link theta::Function Function\endlink: used as coefficients of the components of an observable
 *         specification in a model. Currently, no core plugins are available.</li>
 * <li>\link theta::Minimizer Minimizer\endlink: used by some producers such as maximum likelihood,
 *        profile likelihood methods:
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
 *       <li>\link mle mle \endlink maximum likelihood estimator estimates parameter values and errors using a minimizer
 *             on the negative-log-likelihood function</li>
 *       <li>\link deltanll_intervals deltanll_intervals \endlink interval creation based on the difference in the
 *             negative-log-likelihood function</li> 
 *       <li>\link deltanll_hypotest deltanll_hypotest \endlink creates likelihood-ratio test statistics to find
 *             the critical region for rejecting a "background only" null hypothesis</li>
  *       <li>\link nll_scan nll_scan \endlink scan the likelihood function in one parameter and minimize with respect to all others</li>
 *       <li>\link mcmc_quantiles mcmc_quantiles \endlink Quantile estimator based on Markov-Chain Monte-Carlo to be
 *             used for interval estimation</li>
 *       <li>(not yet implemented:) mcmc_marginal Determine the marginal distribution (as Histogram) for a parameter</li>
 *       <li>\link mcmc_posterior_ratio mcmc_posterior_ratio \endlink analogue of deltanll_hypotest, but integrates over
 *           any free parameters instead of minimizing</li>
 *       <li>\link pseudodata_writer pseudodata_writer \endlink writes out the created pseudo data. Not really a
 *           statistical method, but technically implemented as a Producer as well</li>
 *    </ul>
 * </ul>
 */
 
 
 /**
  * \page extend Extending theta
  *
  * This section assumes that you have read the \ref intro. One important concept introduced there was that of plugins.
  * As rule of thumb, every setting group in a %theta configuration file corresponds to a C++ instance. More
  * specifically, <b>any setting group containing a <em>type="<typename>"</em>
  * setting is used to construct a C++ object of class &lt;typename&gt; via the plugin system.</b>
  *
  * So far, plugins can be defined for following types:
  * <ul>
  * <li>\link theta::HistogramFunction HistogramFunction\endlink: used in the "histogram=..."-setting
  *      in the observables specification of a  model</li >
  * <li>\link theta::Function Function\endlink: used as coefficients of the components of an observable specification in a model or as
  *     additional term in the log-likelihood for a statistical method</li>
  * <li>\link theta::Minimizer Minimizer\endlink: used by some producers such as maximum likelihood, profile likelihood methods</li>
  * <li>\link theta::Run Run \endlink: the "top-level" object constructed by the %theta main program responsible for creating
  *     pseudo data and invocation of the producers</li>
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
  * <li>In the configuration file, make sure to load the shared-object file as plugin.
  *     You can now use the plugin defined as any other %theta component via
  *     a setting group containing type="yourclass";
  *</ol>
  * For examples of plugins, you can have a look at the \c plugins/ directory, which contains the core plugins of %theta.
  *
  * \section extend_example Example: Function plugin
  *
  * To have a simple but complete example, consider you want to use a 1/sqrt(s) prior for a Bayesian method.
  * In the corresponding producer, it is possible to pass a list of function specifications. The first thing
  * to do is to write down the documentation, specifically addressing how it will be configured. Note that
  * the setting group you speficy has to provide enough information to completely specify the instance to create:
  * the constructor will <em>only</em> have accedd to the \ref theta::plugin::Configuration object which essentially
  * contains the settings from the configuration file and the defined observables and parameters, but not more.
  *
  * The setting group should look like this:
  * \code
  *   some_name = {
  *     type = "one_over_sqrt";
  *     parameter = "s";
  *   };
  * \endcode
  * The type setting is always there. This is required by the plugin system to delegate the construction of such an object
  * to the right plugin. The rest of the settings is up to the author of the plugin.
  *
  * For the implementation, it is considered good style to use the plugin type-name as filename and you <em>have to</em>
  * use this name as C++ class name. You should also create a separate header file which contains the class declaration and
  * the documentation (this  is required for the documentation generation). For sake of simplicity, I will omit this
  * complication.
  *
  * The code required for this plugin is:
  * \code
  *
  * using namespace theta;
  * using namespace theta::plugin;
  *
  * class one_over_sqrt: public Function{
  * private:
  *    ParId pid;
  * public:
  *    one_over_sqrt(const Configuration & cfg){
  *        string parameter_name = cfg.setting["parameter"];
  *        pid = cfg.vm->getParId(parameter_name);
  *    }
  *
  *   virtual double operator()(const ParValues & values) const{
  *      return 1.0 / sqrt(values.get(pid));
  *   }
  *
  *  };
  *
  * REGISTER_PLUGIN(one_over_sqrt)
  * \endcode
  *
  * Let's go through this step-by-step.
  *
  * The first lines just include the namespaces \c theta and \c theta::plugin in the lookup in order to save
  * some typing.
  *
  * The class \c one_over_sqrt is defined as derived class of \ref theta::Function. Each class used in the
  * plugin system must be derived from one of the classes of the list given in the introduction of this page.
  *
  * The \c one_over_sqrt class defines a private data member of type \c ParId. \c ParId instances are used to
  * manage parameter identities: each parameter defined in the configuration file corresponds to a certain
  * value of \c ParId. \c ParId instances can be copied, assigned and compared, but this is all what you can
  * do with them. Any additional information about the parameter (i.e., its name, default value and range)
  * is managed with an instance of \ref theta::VarIdManager, see below in the discussion of the constructor.
  *
  * The public constructor must be defined to have a \code const Configuration & \endcode as the only parameter.
  * \c cfg.setting represents the configuration file setting group the instance is constructed from.
  * This can be used to access the settings in this setting group through
  * the \c operator[]. See the documentation of \link theta::SettingWrapper SettingWrapper \endlink and the
  * libconfig documentation for details.
  * The string configured read into \c parameter_name is then used to get the corresponding \c ParId value.
  * This has to be done with an instance of \ref theta::VarIdManager. The currently relevant instance
  * is available through the Configuration instance, \c cfg.vm.
  *
  * Before going on, it is also important to see what's <em>not</em> there:
  * <ul>
  *   <li>There is no code which handles the case that there is no "parameter" setting in \c cfg.setting. There does not have
  *      to be as the call \c cfg.setting["parameter"] will throw an exception which will be caught in the %theta
  *        main program and reported to the user.</li>
  *   <li>There is no explicit type casting of the configuration file setting. This is done implicitely via the overloaded
  *     casting operators of \link theta::SettingWrapper SettingWrapper \endlink (this concept is actually
  *     stolen form libconfig). In case the 
  *     required type does not matched the one in the configuration file, an exception will be thrown, which will be caught
  *     in %theta.</li>
  *   <li>There is no code to handle the case that the user has mis-spelled the parameter name in the configuration.
  *      In this case ... yes, you guessed right: an exception will be thrown by VerIdManager::getParId.</li>
  * </ul>
  *
  * So these two lines of construction code are enough to have robust code with (implicitely provided) proper error handling.
  *
  * Next, we have a look at the \c operator(). This implements the purely virtual
  * specification theta::Function::operator() and is what is called if the function value is requested.
  * From the argument \c values, we can request the value of \c pid and return the result. However, this is
  * not quite correct yet: the function does not provide proper error handling in case the parameter value is
  * smaller than zero. Therefore, this code should be modified such that the method body is:
  * \code
  *   double val = values.get(pid);
  *   if(val <= 0.0) throw MathException("one_over_sqrt: negative argument");
  *   return 1.0 / sqrt(val);
  * \endcode
  *
  * The last line registers the new class at the plugin system with a macro. You have to do this exactly
  * once for every plugin you ant to use (never put this line into a header, only do that in C++ source files.
  * Otherwise, you define two plugins with the same type name and %theta will abort).
  *
  * You have to compile this file as part of a shared object file which is loaded through the \c plugins
  * setting group.
  *
  * One piece is still missing: as the documented in theta::Function::par_ids, this variable
  * should contain all the parameters this function depends on (this is used for optimizations
  * and to define which parameters a model depends on).
  *
  * The completed example (including the split of .hpp and .cpp) is implemented in
  * plugins/one_over_sqrt.hpp and plugins/one_over_sqrt.cpp.
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
  * Using the \c --nowarn option will disable warnings about unused configuration file settings. Only
  * use this if you are sure that the warnings can be savely ignored (in case of a problem, <em>always</em>
  * reproduce it without using this option first).
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
  * \verbatim
  * merge --outfile=merged.db result1.db result2.db result3.db
  * \endverbatim
  * which merges the files result{1,2,3}.db into the output file merged.db. If the output file exists,
  * it will be overwritten.
  *
  * Another possibility is to merge all files matching \c *.db within one directory via
  * \verbatim
  * merge --outfile=merged.db --in-dir=results
  * \endverbatim
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

/** \page testing Testing
 *
 * There are two types of tests in %theta: unit tests which focus on testing the functionality of
 * a single class or method and statistical test cases which compare results obtained from %theta
 * with results analytically obtainable. For the unit tests, look at the source files in the \c test source
 * directory. To run the unit tests, execute
 * <pre>
 * make
 * source setenv.sh
 * make run-test
 * </pre>
 * from the %theta directory.
 *
 * In the following sections, only the statistical tests will be discussed. The source code of these
 * tests can be found in the \c test/test-stat directory.
 *
 * \section testing_counting-nobkg Counting experiment without background
 *
 * As first test case, consider a model with poisson signal around a true mean \f$ \Theta \ge 0 \f$. The outcome of an experiment is completely
 * described by the number of observed events, \f$ n \f$. The likelihood function is given by
 * \f[
 *   L(\Theta | n) = \frac{\Theta^n e^{-\Theta}}{n!}
 * \f]
 * which is maximized by the choice \f$ \Theta = n \f$.
 *
 * A model template is defined in \c test-stat/counting-nobkg.cfg.tpl; the tests will be done for \f$ \Theta=5.0 \f$ (called "low n case" below)
 * and the for \f$ \Theta=10000.0 \f$ (called "asymptotic case" below). Some methods are not expected to have the
 * desired properties for these models. However, this is not what is to be tested here; rather, the implementation
 * of the algorithms is the interest of these tests.
 *
 * \subsection testing_counting-nobkg_mle Maximum likelihood method
 *
 * <em>Test script:</em> <tt>test/test-stat/counting-nobkg-mle.py</tt><br>
 * <em>Relevant plugin classes:</em> \link mle mle \endlink, \link root_minuit root_minuit \endlink
 *
 * 500 pseudo experiments are performed by throwing a Poisson random number around \f$ \Theta \f$ which is passed
 * as number of observed events \f$ n \f$ to the mle implementation.
 *
 * The maximum likelihood method should always estimate \f$ \hat\Theta = n \f$. A small deviation due to numerical evaluation
 * is tolerated. The relative deviation must not exceed \f$ 10^{-4} \f$. In the asymptotic case, also the error estimate
 * \f$ \hat\sigma_{\Theta} \f$ from the minimizer is checked: \f$ \hat\sigma_{\Theta}^2 = \hat\Theta \f$ should hold with
 * a maximum relative error of \f$ 10^{-4} \f$.
 *
 * \subsection testing_counting-nobkg_mcmc_quantiles Bayesian Intervals with Markov-Chain Monte-Carlo 
 *
 * <em>Test script:</em> <tt>test/test-stat/counting-nobkg-mcmc_quant.py</tt><br>
 * <em>Relevant plugin classes:</em> \link mcmc_quantiles mcmc_quantiles \endlink
 *
 * With a flat prior on the signal mean \f$ \Theta \f$, the posterior in
 * case of \f$ n \f$ observed events, \f$ \pi(\Theta | n)\f$, is the same as the likelihood function given above
 * (it is already properly normalized):
 * \f[
 *   \pi(\Theta | n) = \frac{\Theta^n e^{-\Theta}}{n!}.
 * \f]
 *
 * Given an estimate for the q-quantile, \f$ \hat{\Theta}_q \f$, the true value of \f$ q \f$ is given by
 * \f[
 *  q = \int_0^{\hat{\Theta}_q} d\Theta\,\pi(\Theta | n) = \frac{\gamma(n+1, \hat{\Theta}_q)}{\Gamma(n+1)}
 * \f]
 * where \f$ \Gamma \f$ is the gamma function and \f$ \gamma \f$ the incomplete lower gamma function.
 *
 * The tested quantiles correspond to the median, the one-sigma central interval and a 95% upper limit: 0.5, 0.16, 0.84, 0.95.
 * The Markov chain length is 10,000.
 *
 * The test calculates the error on \f$ q \f$ (<em>not</em> on \f$ \hat{\Theta}_q \f$) for 50 pseudo experiments for each of
 * the 4 quantiles. If the deviation is larger than 0.015 (absolute), then the pseudo-experiment
 * is marked as "estimate too low" or "estimate too high"
 * depending whether the requested value of \f$ q \f$ was lower or high than the estimated value. (A pseudo-experiment
 * can have both marks as there are 4 quantiles which are tested.)
 *
 * If there are more than 15 pseudo experiments in either of the categories,
 * the test is considered failed, otherwise, it is considered passed.
 * As additional diagnostics, the number of pseudo experiments marked "too low"
 * and "too high" are printed. In a bias-free method, these
 * two values should be similar, i.e., the value estimated should be sometimes too
 * low and sometimes too high, and not prefer
 * one side. However, no automatic diagnostics is run, as these numbers are small.
 *
 * \subsection testing_counting-nobkg_deltanll "Delta-log-likelihood" confidence intervals
 *
 * <em>Test script:</em> <tt>test/test-stat/counting-nobkg-deltanll_intervals.py</tt><br>
 * <em>Relevant plugin classes:</em> \link deltanll_intervals deltanll_intervals \endlink, \link root_minuit root_minuit \endlink
 *
 * Given the number of observed events, \f$ n \f$, the logarithm of the likelihood ratio between
 * a free value \f$ \Theta \f$ and the value which maximizes the likelihood function (i.e., \f$ \Theta = n \f$) is
 * \f[
 *    \log LR(\Theta | n) = n \log\frac n \Theta - n + \Theta.
 * \f]
 * The minimum is at \f$ \Theta = n \f$ where this logarithm becomes 0.
 *
 * For a given confidence level \f$ c \f$, the interval construction finds the two values of \f$ \Theta \f$,
 * \f$ l_c \f$ and \f$ u_c \f$ with \f$ l_c \le u_c \f$ for which the difference of the log-likelihood to its minimum,
 * \f$ \Delta NLL \f$ corresponds (via the asymptotiv property of the likelihood ratio acording to Wilks' Theorem)
 * to the requested confidence level \f$ c \f$ via the equation
 * \f[
 *   \Delta NLL(c) = \left( \mathrm{erf}^{-1}(c) \right)^2
 * \f]
 * where \f$ \mathrm{erf} \f$ is the error function.
 *
 * For fixed \f$ n \f$ and \f$ c \f$, the values for \f$ \Theta \f$ which yields this difference in negative-log-likelihood is found
 * <ol>
 *  <li>by solving the equation \f$ \log LR(\Theta | n) = \Delta NLL(c)\f$ directly (which has to be done numerically), and</li>
 *  <li>by the generic \link deltanll_intervals deltanll_intervals \endlink method implemented in %theta.</li>
 * </ol>
 *
 * Technically, point 1. is accomplished by an independent python function which expects \f$ n \f$ and \f$ c \f$ as
 * input and applies the bisection method to find the lower and upper value of \f$ \Theta \f$ for which the equation of 1. holds.
 *
 * The requested confidence levels tested correspond to one-sigma and two-sigma interals and the maximum likelihood value, i.e.,
 * confidence levels 0.6827, 0.9545 and 0. The outcome if the method are estimates for the lower and upper values of
 * the intervals for \f$ |Theta \f$. Below, they will be called \f$ l_{1\sigma}, u_{1\sigma}, l_{2\sigma}, u_{2\sigma}, l_{0} \f$.
 *
 * In both cases, the maximum tolerated deviation in the lower and upper interval borders is \f$ 10^{-4}\Theta \f$ .
 *
 * In the asymptotic case, it is also checked that the "one-sigma" and "two-sigma" intervals are where expected in this regime, i.e.,
 * \f{eqnarray*}
 *   (l_{1\sigma} - l_0)^2 &= l_0 \\
 *   (u_{1\sigma} - l_0)^2 &= l_0 \\
 *   (l_{2\sigma} - l_0)^2 &= 4\cdot l_0 \\
 *   (u_{2\sigma} - l_0)^2 &= 4\cdot l_0 \\
 * \f}
 * with a maximum relative difference of \f$ 2\cdot 10^{-2} \f$ , where "relative difference" means dividing the difference of the
 * left and right hand side of these equations by the right hand side (note: the tolerance is chosen relatively large to avoid
 * reporting false positives. This test is mainly to ensure that there is no common bug in the python function for point 1.
 * and the \link deltanll_intervals deltanll_intervals \endlink plugin when calculating \f$ \Delta NLL(c) \f$ ).
 *
 * \section testing_counting-fixedbkg Counting experiment with fixed background mean
 *
 * As second test case, consider a counting experiment with poisson signal with true mean \f$ \Theta \ge 0 \f$ and
 * background mean \f$ \mu \ge 0 \f$ which is assumed to be known. This example is mainly useful to test the correct
 * treatment of a fixed parameter and the error estimates on these parameters.
 *
 * The tests are very similar to the previous section. As there, the values 5.0 and 10000.0 are tested for \f$ \Theta \f$.
 * The values for \f$ \mu \f$ used are 3.0 and 2000.0. For each test case, all four possible combinations of these
 * parameter values are tested.
 *
 * \subsection testing_counting-fixedbkg_mle Maximum likelihood method
 *
 * <em>Test script:</em> <tt>test/test-stat/counting-fixedbkg-mle.py</tt><br>
 * <em>Relevant plugin classes:</em> \link mle mle \endlink, \link root_minuit root_minuit \endlink
 *
 * 500 pseudo experiments are performed by throwing a Poisson random number around \f$ \Theta + \mu \f$ which is passed
 * as number of observed events \f$ n \f$ to the mle implementation.
 *
 * The maximum likelihood method should estimate \f$ \hat\Theta = n - \mu \f$ for \f$ n > \mu \f$ and \f$ \hat\Theta = 0 \f$ otherwise.
 * The maximum tolerated numerical deviation from this result is set to \f$ 10^{-4} \cdot (\Theta + \mu) \f$.
 *
 * In the asymptotic case (\f$ \Theta = 10000 \f$), the error estimate for the minimizer, \f$ \hat{\sigma}_{\Theta} \f$ is checked
 * to fulfill \f$ \hat{\sigma}_\Theta^2 = \Theta + \mu \f$ with a maximum relative error of \f$ 10^{-4} \f$.
 *
 * \subsection testing_counting-fixedbkg_mcmc_quantiles Bayesian Intervals with Markov-Chain Monte-Carlo
 *
 * <em>Test script:</em> <tt>test/test-stat/counting-fixedbkg-mcmc_quant.py</tt><br>
 * <em>Relevant plugin classes:</em> \link mcmc_quantiles mcmc_quantiles \endlink
 *
 * With a flat prior on the signal mean \f$ \Theta \f$ and fixed \f$ \mu \f$, the posterior in case of \f$ n \f$ observed events,
 * \f$ \pi(\Theta | n)\f$, is
 * \f[
 *   \pi(\Theta | n) = \frac{1}{\Gamma(n+1, \mu)} (\Theta + \mu)^n e^{- \Theta - \mu}
 * \f]
 * where \f$ \Gamma \f$ is the "upper" incomplete Gamma function.
 *
 * Given an estimate for the q-quantile, \f$ \hat{\Theta}_q \f$, the true value of \f$ q \f$ is given by
 * \f[
 *  q = \int_0^{\hat{\Theta}_q} d\Theta\,\pi(\Theta | n) = 1 - \frac{\Gamma(n+1, \hat{\Theta}_q + \mu )}{\Gamma(n+1, \mu)}.
 * \f]
 *
 * The tested quantiles correspond to the median, the one-sigma central interval and a 95% upper limit: 0.5, 0.16, 0.84, 0.95.
 * The Markov chain length is 10,000.
 *
 * The test calculates the error on \f$ q \f$ for 50 pseudo experiments for each of
 * the 4 quantiles. If the deviation is larger than 0.015 (absolute), then the pseudo-experiment
 * is marked as "estimate too low" or "estimate too high"
 * depending whether the requested value of \f$ q \f$ was lower or high than the estimated value. (A pseudo-experiment
 * can have both marks as there are 4 quantiles which are tested.)
 *
 * If there are more than 15 pseudo experiments in either of the categories,
 * the test is considered failed, otherwise, it is considered passed.
 * As additional diagnostics, the numbers of pseudo experiments marked "too low"
 * and "too high" are printed.
 *
 * \subsection testing_counting-fixedbkg_deltanll "Delta-log-likelihood" confidence intervals
 *
 * <em>Test script:</em> <tt>test/test-stat/counting-fixedbkg-deltanll_intervals.py</tt><br>
 * <em>Relevant plugin classes:</em> \link deltanll_intervals deltanll_intervals \endlink, \link root_minuit root_minuit \endlink
 *
 * As in case of no background, the likelihood ratio of the likelihood function at \f$ \Theta \f$ and at its minimum is calculated.
 * Now, there are two cases to treat seperately:
 * <ol>
 *  <li> \f$ n >= \mu \f$. In this case, the maximum likelihood estimate for \f$ \Theta \f$ is given by \f$ \hat{\Theta} = n - \mu \f$ </li>
 *  <li> \f$ n < \mu \f$. In this case, the maximum likelihood estimate for \f$ \Theta \f$ is given by \f$ \hat{\Theta} = 0 \f$ </li>
 * </ol>
 *
 * In the first case, the logarithm of the likelihood ratio is given by
 * \f[
 *    \log LR(\Theta | n) = n \log\frac{n}{\Theta + \mu} - n + \Theta + \mu.
 * \f]
 * The minimum is at \f$ \Theta = n - \mu \f$ where this logarithm becomes 0.
 *
 * In the second case, the logarithm of the likelihood ratio is given by
 * \f[
 *  \log LR(\Theta | n) = n \log\frac{\mu}{\Theta + \mu} + \Theta
 * \f]
 *
 *
 * For a given confidence level \f$ c \f$, the interval construction finds the two values of \f$ \Theta \f$,
 * \f$ l_c \f$ and \f$ u_c \f$ with \f$ l_c \le u_c \f$ for which the difference of the log-likelihood to its minimum,
 * \f$ \Delta NLL \f$ corresponds (via the asymptotiv property of the likelihood ratio acording to Wilks' Theorem)
 * to the requested confidence level \f$ c \f$ via the equation
 * \f[
 *   \Delta NLL(c) = \left( \mathrm{erf}^{-1}(c) \right)^2
 * \f]
 * where \f$ \mathrm{erf} \f$ is the error function.
 *
 * For fixed \f$ n \f$ and \f$ c \f$, the values for \f$ \Theta \f$ which yields this difference in negative-log-likelihood is found
 * <ol>
 *  <li>by solving the equation \f$ \log LR(\Theta | n) = \Delta NLL(c)\f$ directly (which has to be done numerically), and</li>
 *  <li>by the generic \link deltanll_intervals deltanll_intervals \endlink method implemented in %theta.</li>
 * </ol>
 *
 * Technically, point 1. is accomplished by an independent python function which expects \f$ n \f$ and \f$ c \f$ as
 * input and applies the bisection method to find the lower and upper value of \f$ \Theta \f$ for which the equation of 1. holds.
 *
 * The requested confidence levels tested correspond to one-sigma and two-sigma interals and the maximum likelihood value, i.e.,
 * confidence levels 0.6827, 0.9545 and 0. The outcome if the method are estimates for the lower and upper values of
 * the intervals for \f$ |Theta \f$. Below, they will be called \f$ l_{1\sigma}, u_{1\sigma}, l_{2\sigma}, u_{2\sigma}, l_{0} \f$.
 *
 * In both cases, the maximum tolerated deviation in the lower and upper interval borders is \f$ 10^{-4}\Theta \f$.
 *
 * \section testing_counting-constraintbkg Counting experiment with background sideband fit
 *
 * As third test case, consider a counting experiment with poisson signal with true mean \f$ \Theta \ge 0 \f$ and
 * a background mean \f$ \mu \ge 0 \f$ which is assumed to be unknown a-priori. The counting is done in two channels:
 * the first channel, \f$ c_1 \f$, is a selection specifically for signal events. The model assumes that the number
 * of observed events follows a Poisson distribution around \f$ \Theta + \mu \f$. In a statistically independent
 * background channel, \f$ c_2 \f$, no signal is expected at all and the model assumes that the number of observed events
 * follows a Poisson distribution around \f$ R\times\mu \f$, where \f$ R \f$ is the acceptance ratio for background events of
 * the two channels. It is assumed that \f$ R \f$ is known.
 *
 *
 */

/** \brief Common namespace for %theta
 */
namespace theta{}
