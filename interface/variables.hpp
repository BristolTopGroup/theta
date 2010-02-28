#ifndef VARIABLES_HPP
#define VARIABLES_HPP

#include <set>
#include <map>
#include <limits>
#include <string>
#include <sstream>
#include <vector>

#include "interface/exception.hpp"
#include "interface/utils.hpp"

namespace theta {

    class VarIdManager;
    
    
    /** \brief To refer to a certain parameter or observable,\c ParId and \c ObsId instances are used throughout %theta.
     *
     *  ParId and ObsId are used internally everyehere where a user would write the parameter / observable name
     *  to refer to a certain parameter / observable.
     *
     *  The \c VarId class defines equality and less-than order relation, so they can be used as key
     *  in a map.
     *
     *  Any associated informations such as default value and ranges are stored
     *  in a VarIdManager instance. Concrete parameter values are defined via an instance
     *  of the ParValues class.
     */
    template<typename tag>
    class VarId{
    friend class VarIdManager;
    friend class ParValues;
    public:
        //@{
        /** \brief Implements the order and equality semantics.
         */
        bool operator==(const VarId & rhs) const{
            return id==rhs.id;
        }
        bool operator!=(const VarId & rhs) const{
            return id!=rhs.id;
        }        
        bool operator<(const VarId & rhs) const{
            return id<rhs.id;
        }
        //@}
        
        /** Creates in invalid VarId which will evaulate to false.
         */
        VarId(): id(-1){}
    private:
        int id;
        VarId(int i): id(i){}
    };
    
    // An alternative to tag structs would be using common inheritance from
    // a (instantiable) VarId base class.
    //
    // However, using typedefs with tags instead will create truely new C++ types
    // and will make comparion of ObsId and VarId objects impossible, as it should
    // be (CCS, Item 14).
    /// \brief Empty tag struct to create ParId from the VarId template.
    struct ParIdTag{};
    /// \brief Empty tag struct to create ObsId from the VarId template.
    struct ObsIdTag{};
    
    /** Identification object for model parameters.
     * 
     * ParId instances are usually managed via a VarIdManager instance, where
     * additional information about each parameter is stored.
     * 
     * \sa VarId ObsId
     */    
    typedef VarId<ParIdTag> ParId;
    
    /** \brief Identification object for model observables.
     * 
     * ObsId objects are typically managed via a VarIdManager instance, where 
     * additional information about each observable is stored.
     * 
     * \sa VarId ParId
     */    
    typedef VarId<ObsIdTag> ObsId;
    
    /** A container for \c ParId or \c ObsId values. Set the template parameter id_type
     * to either \c ParId or \c ObsId
     *
     * The interface allows an STL-like iteration over the VarIds contained in this object.
     * The iteration will visit the elements in their natural order.
     * 
     * \tparam id_type The type to build the container for. Only ObsId and ParId are used here.
     */
    template<class id_type>
    class VarIds {
    public:
        /// \brief A STL compliant forward iterator 
        typedef typename std::set<id_type>::const_iterator const_iterator;

        /// \brief Get the an iterator pointing to the first element. 
        const_iterator begin()const {
            return vars.begin();
        }
        
        /// \brief Get the an iterator pointing past the last element.
        const_iterator end() const {
            return vars.end();
        }

        /** \brief Insert a new id.
         *
         * \param id The id object to insert. 
         * \return \c true, if an insertion actually took place and \c false if the id was already contained. 
         */
        bool insert(const id_type & id) {
            return vars.insert(id).second;
        }
        
        /** \brief Insert new ids.
         *
         * Insert [first, last) in this container.
         */
        void insert(const_iterator first, const_iterator last) {
            vars.insert(first, last);
        }

        /** \brief Test whether an id is contained.
         * 
         * \param id The id object to test. 
         * \return Whether \c id is contained.
         */ 
        bool contains(const id_type & id)const {
            return vars.find(id) != vars.end();
        }

        /** \brief Test equality with other VarIds object.
         *
         * Two VarIds are the same if and only if the set of contained VarId s is the same.
         */
        bool operator==(const VarIds<id_type> & rhs) {
            return vars == rhs.vars;
        }

        /// The number of contained ids
        size_t size() const {
            return vars.size();
        }
    private:
        std::set<id_type> vars;
    };
    
    /// \brief Template instantiation for a set of observables
    typedef VarIds<ObsId> ObsIds;
    /// \brief Template instantiation for a set of parameters
    typedef VarIds<ParId> ParIds;

    class ParValues;
    
    /** \brief Manager class for parameter and observable information
     *
     * This class provides methods to save the information given in the "parameters"
     * and "observables" setting groups.
     *
     * This class
     * <ul>
     * <li>keeps track of the association between parameter / observable names and ParId / ObsId objects</li>
     * <li>saves the configured range and binning for the observables and the range and default value for parameters</li>
     * </ul>
     *
     * Note that there does not exist any global "current value" of a variable.
     */
    class VarIdManager {
        friend class ParValues;
    public:
        //@{
        /** \brief Creates a new parameter or observable ids (ParId, ObsId) and associates it with the given name.
         *
         * If the name is already used for another parameter / observable, an InvalidArgumentException is thrown.
         */
        ParId createParId(const std::string & name, double def = 0.0, double xmin=-std::numeric_limits<double>::infinity(), double xmax=std::numeric_limits<double>::infinity());
        ObsId createObsId(const std::string & name, size_t nbins, double xmin, double xmax);
        //@}
        
        //@{
        /** \brief Returns wheher the given name is already used as parameter / observable name.
         *
         * Note that parameters and obserables are different things in theta and it is possible
         * (although not recommended) to have the same name for a parameter and an observable.
         *
         * Names are case-sensitive.
         */
        bool parNameExists(const std::string & name) const;
        bool obsNameExists(const std::string & name) const;
        //@}
        
        //@{
        
        /** \brief Set the range an default value of a parameter.
         *
         * This function only exists for parameters. Observables are fixed once created as this information
         * it typically used for constructing Histograms.
         *
         * So every code alles get_range(const ParId &) or get_default must make sure that
         * changes are respected, whereas code for observables can assume that once they
         * get the information about bins and range, it will never change.
         *
         * Throws an NotFoundException if the parameter does not exist an an InvalidArgumentException
         * if def is not conatined in the interval [low, high].
         */
        void set_range_default(const ParId & pid, double low, double high, double def);
        
        //@{
        /** \brief Return the name of the given ParId or ObsId.
         *
         * If the id is not known, a NotFoundException is thrown.
         */
        std::string getName(const ParId & id) const;
        std::string getName(const ObsId & id) const;
        //@}
        
        //@{
        /** \brief Return default value and range for a parameter identified by the  ParId id.
         */
        double get_default(const ParId & id) const;
        const std::pair<double, double> & get_range(const ParId & id) const;
        //@}
        
        /** \brief Return all default values.
         */
        ParValues get_defaults() const;
        
        //@{
        /** \brief Return the number of bins and range for an observable identified by the ObsId id.
         */
        size_t get_nbins(const ObsId & id) const;
        const std::pair<double, double> & get_range(const ObsId & id) const;
        //@}
        
        //@{
        /** \brief Return the ParId / ObsId with the given name
         *
         * If the name is not known, a NotFoundException is thrown.
         *
         * If you merely want to test whether a name already exists, use parNameExists and obsNameExists
         */
        ParId getParId(const std::string & name) const;
        ObsId getObsId(const std::string & name) const;
        //@}
        
        //@{
        /** Returns all currently defined ParId or ObsId identifiers as ParIds or ObsIds
         */
        ParIds getAllParIds() const;
        ObsIds getAllObsIds() const;
        //@}
        
        /** \brief Create an empty VarIdManager with no registered variables.
         */
        VarIdManager(): next_pid_id(0), next_oid_id(0) {
        }

    private:
        //ParIds:
        std::map<ParId, std::string> pid_to_name;
        std::map<std::string, ParId> name_to_pid;
        std::map<ParId, double> pid_to_default;
        std::map<ParId, std::pair<double, double> > pid_to_range;
        int next_pid_id;
        //ObsIds:
        std::map<ObsId, std::string> oid_to_name;
        std::map<std::string, ObsId> name_to_oid;
        std::map<ObsId, std::pair<double, double> > oid_to_range;
        std::map<ObsId, size_t> oid_to_nbins;
        int next_oid_id;
        
        //prevent copying (Model will copy it, though):
        VarIdManager(const VarIdManager &);
    };

    /** \brief A mapping-like class storing parameter values.
     * 
     * Conceptually, represents a mapping from ParId instances to double values.
     * It can only hold non-NAN values (a NAN-value is treated as non-existent). 
     *
     * Used to pass <b>actual</b> values of parameters to functions, as opposed to
     * passing a set of parameters, where parameter <i>identity</i> is sufficient.
     * For the latter, ParIds and ObsIds objects are used.
     */
    class ParValues {
    public:
        /** \brief Default constructor which creates an empty container.
        */
        ParValues():values(10, NAN){}
        
        /** Constructor for \c VarValues which will hold values of \c vm.
         *
         * This is semantically equivalent to the default constructor. Using this
         * constructor makes possible some optimizations.
         */
        ParValues(const VarIdManager & vm): values(vm.next_pid_id, NAN){}
        
        /** \brief Set a value.
         *
         * Set the value of the ParId \c pid to \c val. Setting a parameter 
         * to NAN means to delete it from the list (i.e., get() will throw a
         * NotFoundException for that parameter). This makes it possible to "clear" a parameter
         * from a VarValues instance after setting it.
         *
         * Returns a reference to this \c ParValues object to allow 
         * chaining calls like \c values.set(a, 0.0).set(b, 1.0).set(c, 2.0) ...
         *
         * \param pid Identified the parameter to assign a new value to.
         * \param value The new value for the parameter.
         */
        ParValues & set(const ParId & pid, double val){
            const int id = pid.id;
            if(id >= (int)values.size()){
                values.resize(id+1, NAN);
            }
            values[id] = val;
            return *this;
         }
         
         /** \brief Set all values contained in rhs.
          *
          * This is equivalent to calling set(pid, val) for each pair (pid, val) contained in rhs.
          */
         void set(const ParValues & rhs){
             if(rhs.values.size() > values.size()){
                 values.resize(rhs.values.size(), NAN);
             }
             for(size_t i=0; i<rhs.values.size(); ++i){
                 if(std::isnan(rhs.values[i]))continue;
                 values[i] = rhs.values[i];
             }
         }
        
        /** \brief Add a value to a parameter.
         *
         * Same as \c set(pid, get(pid) + delta), but faster. Throws NotFoundException if not value
         * was set for \c pid before.
         *
         * \param pid The parameter to change.
         * \param delta The value to add to the parameter. 
         */
        void addTo(const ParId & pid, double delta){
            const int id = pid.id;
            if(id >= (int)values.size() || isnan(values[id])){
                throw NotFoundException("VarValues::addTo: given ParId not found.");
            }
            values[id] += delta;
        }

        /** \brief Retrieve the current value of a parameter.
         *
         *  Throws a NotFoundException if no value was set for \c pid in this \c ParValues.
         *
         *  \param pid The parameter for which the value should be returned.
         *  \return The current value for the parameter \c pid.
         */
        double get(const ParId & pid) const{
            double result;
            const int id = pid.id;
            if(id >= (int)values.size() || isnan(result = values[id])){
                std::stringstream ss;
                ss << "VarValues::get: given VarId " << id << " not found";
                throw NotFoundException(ss.str());
            }
            return result;
        }

        /** \brief Returns whether \c pid is contained in this VarVariables.
         */
        bool contains(const ParId & pid) const{
            const int id = pid.id;
            return id < (int)values.size() and not isnan(values[id]);
        }

        /** \brief Return all \c ParIds of the variables in this \c VarValues.
         */
        ParIds getAllParIds() const;

    private:
        //values are stored using the VarId.id as index
        std::vector<double> values;
    };

}

#endif 
