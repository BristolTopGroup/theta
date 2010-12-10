#ifndef PHYS_HPP
#define PHYS_HPP

#include "interface/decls.hpp"
#include "interface/variables.hpp"
#include "interface/plugin.hpp"
#include "interface/distribution.hpp" 

#include "interface/histogram-function.hpp"

#include <vector>
#include <string>
#include <limits>
#include <set>
#include <map>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_map.hpp>

namespace theta {
    
    /** \brief A real-valued function which depends on one or more parameters
     *
     * This is the base class for function plugins.
     */
    class Function{
    public:
        /// Define this as the base_type for derived classes; required for the plugin system
        typedef Function base_type;
        
        /** \brief Evaluate the function at the given parameter values.
         *
         * @return The function value at \c v.
         */
        virtual double operator()(const ParValues & v) const = 0;

        /** \brief Evaluate the function, using the parameter values given as array of doubles.
         *
         * This does the same as operator()(const ParValues&), however, it takes a pointer
         * to an array of doubles instead.
         *
         * This function is provided to make it easier to provide an interface to other libraries, which
         * do not need to know about the theta-style of variable handling (i.e.,
         * \c ParId, \c ParValue, \c VarIdManager classes).
         *
         * The translation from this "external" array format into the internal (i.e.,
         * \c ParValues) format is done by simultaneously iterating over the
         * ParIds as returned by getParameters() and the given array \c x.
         * (As iteration over a \c ParIds object always yields the same order of parameters,
         * this mapping is well-defined.)
         */
        double operator()(const double * x) const{
            size_t i=0;
            for(ParIds::const_iterator it=par_ids.begin(); it!=par_ids.end(); ++it, ++i){
                assert(!std::isnan(x[i]));
                pv.set(*it, x[i]);
            }
            return operator()(pv);
        }


        /** \brief Returns the parameters this function depends on
         */
        const ParIds & getParameters() const{
            return par_ids;
        }

        /** \brief The number of parameters this function depends on
         *
         * Same as \c getParameters().size().
         */
        size_t getnpar() const{
            return par_ids.size();
        }
        
        /// Declare destructor virtual as polymorphic access to derived classes will happen.
        virtual ~Function(){}
    protected:
        /** \brief The parameters this function depends on
         *
         * Has to be set correctly by derived classes
         */
        ParIds par_ids;
        
        /** \brief Assignment operator. For use by derived classes
         *
         */
        //Do not use default implementation, as ParValues pv cannot be assigned (and they do not need to be ...).
        void operator=(const Function & rhs){
            par_ids = rhs.par_ids;
        }
        
    private:
        mutable ParValues pv; //saving this class-wide and not in operator()(const double*) saves quiet some time ...
    };
    
    
    /** \brief Contains data for one or more observables
     *  
     * A data object can be constructed:
     * -# "by hand": use the default constructor and set data Histograms for a number of observables
     * -# by using the DataFactory, which parses a configuration setting and typically reads data from root files
     * -# by sampling from a \c Model using the \c samplePseudoData method.
     *
     * After construction, the Data object is typically used to get a likelihood function from a Model.
     *
     * \sa Model::getLikelihood(Data) DataFactory Model::samplePseudoData
     */
    class Data {
    public:
        /** \brief Returns all obs_ids for which any data was added using addData(obs_id).
         */
        ObsIds getObservables() const;

        /** \brief Access the histogram with an observable
         *
         * The const version is usually only used to read a previously set
         * Histogram. If no Histogram is saved for the supplied observable id,
         * a NotFoundException will be thrown from the const version.
         */
        //@{
        Histogram & operator[](const ObsId & id){
            return data[id];
        }
        const Histogram & operator[](const ObsId & id) const{
            std::map<ObsId, Histogram>::const_iterator it = data.find(id);
            if(it==data.end()) throw NotFoundException("Data::operator[]() const: no data found for given ObsId");
            return it->second;
        }
        //@}

    private:
        std::map<ObsId, Histogram> data;
    };
    
    /** \brief A data-providing class, can be used as base class in the plugin system
     *
     * DataSource classes are used as part of a run, which, for each pseuso
     * experiment, calls the DataSource::fill function to get the pseudo data.
     */
    class DataSource: public theta::plugin::ProductsTableWriter{
    public:
        
        /// Define this as the base_type for derived classes; required for the plugin system
        typedef DataSource base_type;
        
        /** \brief Exception class to be thrown by fill
         */
        class DataUnavailable{};
        
        /** \brief Returns the observables this DataSource provides data for
         *
         * This stays the same over the lifetime of the instance.
         *
         * The return value is the same as \c obs_ids in
         * \code
         * Data data;
         * this->fill(data);
         * ParIds obs_ids = values.getObservables();
         * \endcode 
         */
        ObsIds getObservables() const{
            return obs_ids;
        }
        
        /** \brief Fill the provided Data instance with data
         *
         * This method is called exactly once per event. It sets
         * the histograms of the provided observables and lets everything else unaffected.
         *
         * It can throw a DataUnavailable exception if no more data is available and the
         * run should end.
         */
        virtual void fill(Data & dat, Run & run) = 0;
        
        /// Declare destructor virtual as polymorphic access to derived classes will happen.
        virtual ~DataSource(){}
        
    protected:
        /// proxy to ProductsTableWriter constructor for derived classes
        DataSource(const theta::plugin::Configuration & cfg): theta::plugin::ProductsTableWriter(cfg){}
        ObsIds obs_ids;
    };
    

}

#endif
