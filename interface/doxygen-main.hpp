/* note: this file is not for inclusion in C++ code. It is only here
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
 * %theta make it easier to treat commonly arising questions such as the treatment of nuisance
 * parameters, coverage tests, luminosity scans, large-scale production of test statistic points,
 * and many more.
 *
 * The documentation is split into several pages. If you are new to %theta, read them in this order:
 * <ol>
 *   <li>\subpage getting_started "Getting Started" explains how to obtain and compile %theta</li>
 *   <li>\subpage intro Introduction describes how to run %theta; the example discussed there gives a good first
 *                 overview of how %theta works.</li>
 *   <li>\subpage arch "General architecture of theta" describes more in-depth the architecture of %theta and provides
 *            a good entry point if you want to extent %theta</li>
 *   <li>\subpage design "Design Goals of theta" contains some thoughts about what the code of %theta should be like.
 *       You should read that either if you want to contribute code to %theta or if you want to know what makes %theta different.</li>
 * </ol>
 *
 * \section ack Acknowledgement
 *
 * I would like to thank the authors of the software packages used by %theta:
 * <ul>
 * <li>\link http://www.hyperrealm.com/libconfig/libconfig.html libconfig \endlink A
 *      well-written, well-documented C/C++ library for processing configuration files with a very simple and elegant API.</li>
 * <li>\link http://www.chokkan.org/software/liblbfgs/ liblbfgs \endlink An implementation of
 *     the Limited-memory Broyden-Fletcher-Goldfarb-Shanno minimization algorithm</li>
 * </ul>
 * These libraries are included in the distribution of %theta.
 *
 * Furthermore, some parts of the random number algorithm code and for the matrix code
 * have been copied from the excellent \link http://www.gnu.org/software/gsl/ GNU Scientific Library (GSL) \endlink.
 *
 * \section license License
 *
 * %theta is licensed under the \link http://www.gnu.org/copyleft/gpl.html GPL \endlink.
 */


/** \page getting_started Getting Started
 *
 * \section obtaining Obtaining theta
 *
 * %theta is available as source-code distribution via subversion only. The latest version can be obtained by running
 * <pre>
 * svn co https://ekptrac.physik.uni-karlsruhe.de/svn/theta/trunk theta
 * </pre>
 *
 * \section builiding Building theta
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
 * your chances are good that it compiles under many different versions.
 *
 * \subsection without_cmssw Without CMSSW
 *
 * NOTE: this subsection will become obsolete soon.
 *
 * Be sure to get all the external dependencies and install them either system-wide ore somehwere they can
 * be found during compile-time and run-time:
 * <ol>
 * <li>Boost, including the jam build system</li>
 * <li>sqlite3</li>
 * </ol>
 * (If you install the libraries in a non-standard location, you have to adapt the \c Jamroot file)
 * and run \c bjam. The main executable to run is \c theta. Where it is located depends on the
 * toolchain used. It is put to some location like bin&lt;toolchain&gt;/&lt;releas-variant&gt;/theta.
 */

 /**
 * \page intro Introduction
 *
 * %Theta is about modeling and statistical inference. For %theta, "model" means
 * a specification of the probability density of one or more observables as function of
 * some parameters, including possible probability densities of the parameters. To make this
 * more clear, let us consider a more concrete example: suppose you are searching for a new particle.
 *
 * After a sophsticated event selection, you have events containing candidates of your new particle.
 * For each of these events, you can reconstruct the mass. From your Monte-Carlo simulation,
 * you conclude that your signal has a distribution in this reconstructed mass in the
 * form of a gaussian with mean 1000 GeV/c^2 and width 250 GeV/c^2, whereas your background
 * model (probably from some signal-free data sideband) is expected to be flat in this region (please don't
 * tell me that it is not realistic, I know ;-) ). From a background fit to a sideband you know
 * that your background poisson mean in the signal region is 1600 +- 200 events. The model of your signal
 * allows for a large variety of signal cross sections; the only thing you know is that the standard
 * model predicts no signal at all.
 *
 * There are some questions everyone keeps asking you:
 * <ol>
 * <li>At some fixed integrated luminosity, and assuming that the SM is true, what is your upper limit
 *  on the signal cross section?</li>
 * <li>How does this behave as function of luminosity?</li>
 * <li>Assume that your signal has cross section such that in the mean case 200 signal events pass your event selection.
 *    If it was actually there, could you find it? More precisely:
 *    what will a likelihood-ratio hypothesis test with the null-hypothesis being the standard model tell you about the p-value
 *    of some data (real or simulated)?
 * </ol>
 *
 * For this introduction, only the question of finding the signal if it is actually there will be addressed
 *
 * Now, have a look at the <tt>examples/gaussoverflat.cfg</tt> configuration file. (For now, do not
 * care about syntax too much, see \link config link below \endlink for a detailed description).
 *
 * At the top of the configuration file, a model with the name "gaussoverflat" is defined. It is
 * exactly the model described above: there is one observable "mass" with the range [500, 1500] and
 * 200 bins. Note that theta does not care at all about units and that observables are <em>always</em> binned (of course,
 * you can always make the binning very fine). The parameter this model depends on are defined next: "s" is
 * the (poisson) mean number of signal events after your selection, "b" is the number of
 * background events. Their "default" values are set to the scenario in question. The range is mainly
 * used as contraint for fits. You usually define it as large as physically makes sense. In this case, it makes sense to restrcit the parameters
 * to positive values.
 *
 * Next, (line with <tt>mass=</tt>), the expectation for the "mass" observable is specified. As discussed above,
 * it is a linear combination of s signal events which are gaussian and b background events which are flat. I think
 * you will find the corresponding statements in the configuration file now.
 *
 * After the observable specification, there is a list of constraints. Constraints
 * specified in the model will be used for
 * <ul>
 * <li>as additional term in the likelihood function (Bayesianically, these are priors)</li>
 * <li>If throwing pseudo experiments, they are used to choose the parameters' values for pseudo data creation (if no contraint is
 *      defined for a parameter, always its default value is used for pseudo data generation).
 * </ul>
 * The "constraint" settings block concludes the definitiopn of the model. The next thing
 * to take care of is the hypothesis test. This is done by the "hypotest" settings block.
 *
 * This block defines a statistical method (also called "producer", as it produces
 * results) of type "lnQ-minimize". This module expectes two more settings: "signal-plus-background"
 * and "background-only". They are both setting blocks which specify constraints to apply to
 * get these two model varaints from the original model. In this case, the "background-only" model is given
 * by the constraint "{s=0.0;}".For the "signal-plus-background", no constraints have to be applied, therefore, an empty settings block
 * is given ("{}"). So whenever this module runs, it will be provided with a model and data. It will
 * <ol>
 * <li> Construct the likelihood function of the model, given the data.</li>
 * <li> Minimize the negative logarithm of the likelihood function with the "signal-plus-background" constraint.</li>
 * <li> Minimize the negative log-likelihood function with the "background-only" constraint.</li>
 * <li> save the difference of the outcome of the two previous steps. (For large event numbers, twice the
 *     square root of this value is a very good approximation for the significance with which the "background-only"
 *     null-hypothesis can be rejected.)</li>
 * </ol>
 *
 * Having configured the model and a statistical method, we have to glue them together. In this case, we want to make
 * pseudo experiments. This is done by the "main" settings block. It defines a plain run
 * ("plain" means: throw pseudo data from the model and run the producers, nothing more
 * complicated. Other types of runs can do parameter scans). We have to specify which model to use, which producers
 * to run and how many pseudo experiments you wish to perform. The results will be written
 * as SQL %database to a file with the path specified with "result-file".
 *
 * You can execute the run by specifying the configuration filename:
 * <pre>
 * bin/theta examples/gaussoverflat.cfg
 * </pre>
 * theta will execute the "main" run by default. You can also specify the run name 
 * as the second command line argument.
 *
 * \section pseudodata Generation of pseudo data
 *
 * If pseudo data is thrown according to a model, it proceeds in several steps:
 * <ol>
 *  <li>Determine a random value for each model parameter. This is done with a constraint, if one exists for this parameter. Otherwise,
 *        the default value from the parameter definition is used. In any case, the values of the parameters used will be stored
 *        in the parameter table (see below).</ul>
 *  <li>For each observable, use these parameters to evaluate the HistogramFunction and Histogram coefficients</li>
 *  <li>For each observable, add all components (=histograms determined in the previous step) with the coefficients from the previous step</li>
 *  <li>For each observable, draw a poisson-random sample from the obtained histogram</li>
 * </ol>
 *
 * Note that in step 2., Histograms might be fluctuated to within its uncertainties; for an example see
 * ConstantHistogramFunctionError.
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
 
 
/** \page arch Architecture of %theta
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
 *
 * To make that possible, a plugin system was created. Although it does not permit to replace
 * every possible class just mentioned, it allows to define your own derived classes of:
 * <ul>
 * <li>\link theta::HistogramFunction HistogramFunction\endlink: used in the observables specification of a model</li>
 * <li>\link theta::Function Function\endlink: used as coefficients of HistogramFunctions in a model</li>
 * <li>\link theta::Minimizer Minimizer\endlink: used by some producers such as maximum likelihood, profile likelihood methods</li>
 * <li>\link theta::Run Run \endlink: the top-level object which invokes the pseudo data creation and producers</li>
 * <li>\link theta::Distribution Distribution\endlink: used in model constraints or as priors in a statistical method</li>
 * <li>\link theta::Producer Producer\endlink: statistical method called by a Run object</li>
 * </ul>
 *
 * Note that this is a conclusive enumeration. In particular, it is <i>not</i> possible (at least for now)
 * to write plugins for the \link theta::Model Model \endlink, \link database::Database Database\endlink,
 * or random number generator.
 *
 * For each of these different component classes, there can be as many specific implementations
 * as you want. The different implementations for one class are distinguished by
 * the "type=..." line in the configuration: A central registry knows
 * whom to ask if you request, say, a HistogramFunction with type="fixed-gauss" and invokes
 * a so-called "Factory" class.
 *
 * This architecture has consequences for the documentation:
 * <ol>
 * <li>For each of these components, there is an abstract C++ type. However, this type is kept
 *   "abstract" also in the sense that it cannot provide much documentation.</li>
 * <li>Specific types of these components are documented at the concrete (derived) class</li>
 * <li>The configuration is documented at the corresponding factory classes (as it is there
 *    where the configuration is processed).</li>
 * </ol>
 * So if you want to know something about the type="fixed-gauss" HistogramFunction, which
 * is provided by the FixedGaussFunctionFactory, you can find documentation for that at different places:
 * <ol>
 * <li>The documentation for the configuration directives can be found at the Factory class. This is
 *  the place you might want to look first as it gives an impression of what this class can do.</li>
 * <li>The Factory will produce an instance of some subclass of HistogramFunction, in this case ConstantHistogramFunction.
 *   Everything specific to this subclass is documented at ConstantHistogramFunction.</li>
 * <li>As ConstantHistogramFunction is a subclass of the abstarct type HistogramFunction, some documentation
 *  about HistogramFunction in general can be found there.</li>
 * </ol>
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

/** \brief Common namespace for almost all classes of %Theta.
 *
 * The only exception are %database function which are located in the
 * database namespace.
 */
namespace theta{}

