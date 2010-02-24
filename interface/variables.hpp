#ifndef _VARIABLES_HPP
#define	_VARIABLES_HPP

#include <set>
#include <map>
#include <limits>
#include <string>
#include <sstream>
#include <vector>
#include <cmath>

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
        bool operator==(const VarId & rhs) const{
            return id==rhs.id;
        }
        bool operator!=(const VarId & rhs) const{
            return id!=rhs.id;
        }        
        bool operator<(const VarId & rhs) const{
            return id<rhs.id;
        }
        /*bool operator>(const VarId & rhs) const {
            return id>rhs.id;
        }
        bool operator<=(const VarId & rhs)const{
            return id<=rhs.id;
        }
        bool operator>=(const VarId & rhs)const{
            return id>=rhs.id;
        }*/
        operator bool(){
            return id>=0;
        }
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

        bool operator==(const VarIds<id_type> & rhs) {
            return vars == rhs.vars;
        }

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
    
    /** Manager class for variable idendities (both \c ParId and \c ObsId ).
     *
     * TODO: rewrite this doc, not recent any more ...
     * As one wishes to express that two different objects (say two Funciton objects) take
     * the same variable as argument, one has to manag variable identities. That is, if I have two Function objects
     * f1(v1, v2) and f2(v2, v3) I need to express somehow that the v2 in the argument list of f1 and v2 in
     * the argument list of f2 are actually the same variable.
     *
     * A possibility would be to assign each variable an id number, for example v1_id = 0, v2_id = 1, v3_id = 2.
     * However, keeping track of those consistently through the whole program will be a mess.
     *
     * The idea of assiging variables an id is kept: for each variable, an identifier of type VarId is created which, together
     * with the VarIdManager class, identifies uniquely a variable.
     *
     * Note that no space is allocated for a variable by this class, it is merely managing
     * their idendity (there is no process-wide "current value" for a given variable). Managing concrete
     * values of a set of variables is done by VarValues.
     */

    class VarIdManager {
//        friend class Model;
        friend class ParValues;
    public:
        /** Creates a new variable id and associates it with the given name.
         * If the name is already used for another variable, the corresponding Id is returned,
         * if the specification (default and range) match. If the variable name exists but
         * the specification does not match, an InvalidArgumentException
         * is thrown. */
        ParId createParId(const std::string & name, double def = 0.0, double xmin=-std::numeric_limits<double>::infinity(), double xmax=std::numeric_limits<double>::infinity());
        ObsId createObsId(const std::string & name, size_t nbins, double xmin, double xmax);
        
        bool parNameExists(const std::string & name) const;
        bool obsNameExists(const std::string & name) const;
        
        bool varIdExists(const ParId & id) const;
        bool varIdExists(const ObsId & id) const;
        
        /** Returns the name of the given variable id. If the id is not known,
            a NotFoundException is thrown.
         */
        std::string getName(const ParId & id) const;
        std::string getName(const ObsId & id) const;
        
        /** Parameters have a range and a default value, whereas observables have a range and a number of bins:
        */
        double get_default(const ParId & id) const;
        const std::pair<double, double> & get_range(const ParId & id) const;
        size_t get_nbins(const ObsId & id) const;
        const std::pair<double, double> & get_range(const ObsId & id) const;
        
        /** Returns the VarId for the given variable name. If the name is not known,
         *  a NotFoundException is thrown.
         */
        ParId getParId(const std::string & name) const;
        ObsId getObsId(const std::string & name) const;
        
        /** Returns all currently defined ParIds or ObsIds
         */
        ParIds getAllParIds() const;
        ObsIds getAllObsIds() const;
        
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
         * to NAN has no effect (i.e., it is the same as not calling this function at all).
         *
         * Returns a reference to this \c ParValues object to allow 
         * chaining calls like \c values.set(a, 0.0).set(b, 1.0).set(c, 2.0) ...
         *
         * \post get(pid) == val
         * \post contains(pid)
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

        /** \brief Returns whether vid is contained in this VarVariables.
         */
        bool contains(const ParId & pid) const{
            const int id = pid.id;
            return id < (int)values.size() and not isnan(values[id]);
        }

        /** \brief Return all \c ParIds of the variables in this \c VarValues.
         */
        ParIds getAllParIds() const;

        /* \brief Adds the given variables to this container.
         * 
         * Throws an InvalidArgumentException exception if a value was
         * already contained in this Varvalues. In this case, no guarantee about
         * which variables have already been set are made.
         * 
         * \param values The parameter values to add to this object.
         */
        //void addNew(const ParValues & values);
    private:
        std::vector<double> values;
    };

}

#endif	/* _VARIABLES_HPP */
