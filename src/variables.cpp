#include "interface/variables.hpp"
#include "interface/exception.hpp"

#include <sstream>
#include <cmath>

using namespace theta;
using namespace std;

ParId VarIdManager::createParId(const std::string & name, double def, double min, double max) {
    if (parNameExists(name)) {
        //check default, min and max:
        ParId id = getParId(name);
        if(get_default(id)!=def or get_range(id).first!=min or get_range(id).second!=max){
            stringstream ss;
            ss << "VarIdManager::createParId: name '"<< name <<"' already in use with different specification.";
            throw InvalidArgumentException(ss.str());
        }
        return id;
    }
    ParId result(next_pid_id);
    next_pid_id++;
    pid_to_name[result] = name;
    name_to_pid[name] = result;
    pid_to_range[result] = make_pair(min, max);
    pid_to_default[result] = def;
    return result;
}

ObsId VarIdManager::createObsId(const std::string & name, size_t nbins, double min, double max) {
    if (obsNameExists(name)) {
        ObsId id = getObsId(name);
        if(get_nbins(id)!=nbins or min!=get_range(id).first or max!=get_range(id).second){
            stringstream ss;
            ss << "VarIdManager::createObsId: name '" << name << "' already in use with different specification.";
            throw InvalidArgumentException(ss.str());
        }
        return id;
    }
    ObsId result(next_oid_id);
    next_oid_id++;
    oid_to_name[result] = name;
    name_to_oid[name] = result;
    oid_to_range[result] = make_pair(min, max);
    oid_to_nbins[result] = nbins;
    return result;
}

bool VarIdManager::parNameExists(const std::string & name) const {
    return name_to_pid.find(name) != name_to_pid.end();
}

bool VarIdManager::obsNameExists(const std::string & name) const {
    return name_to_oid.find(name) != name_to_oid.end();
}

bool VarIdManager::varIdExists(const ParId & id) const {
    return pid_to_name.find(id) != pid_to_name.end();
}

bool VarIdManager::varIdExists(const ObsId & id) const {
    return oid_to_name.find(id) != oid_to_name.end();
}

std::string VarIdManager::getName(const ParId & id) const {
    std::map<ParId, std::string>::const_iterator it = pid_to_name.find(id);
    if (it == pid_to_name.end()) {
        throw NotFoundException("VarIdManager::getVarname: did not find given VarId.");
    }
    return it->second;
}

std::string VarIdManager::getName(const ObsId & id) const {
    std::map<ObsId, std::string>::const_iterator it = oid_to_name.find(id);
    if (it == oid_to_name.end()) {
        throw NotFoundException("VarIdManager::getVarname: did not find given VarId.");
    }
    return it->second;
}

VarIdManager::VarIdManager(const VarIdManager & rhs): pid_to_name(rhs.pid_to_name), name_to_pid(rhs.name_to_pid),
        pid_to_default(rhs.pid_to_default),  pid_to_range(rhs.pid_to_range), next_pid_id(rhs.next_pid_id),
        oid_to_name(rhs.oid_to_name), name_to_oid(rhs.name_to_oid), oid_to_range(rhs.oid_to_range), oid_to_nbins(rhs.oid_to_nbins), next_oid_id(rhs.next_oid_id)
        {}

ParId VarIdManager::getParId(const std::string & name) const {
    std::map<std::string, ParId>::const_iterator it = name_to_pid.find(name);
    if (it == name_to_pid.end()) {
        stringstream ss;
        ss << __FUNCTION__ << ": did not find variable '" << name << "'";
        throw NotFoundException(ss.str());
    }
    return it->second;
}

ObsId VarIdManager::getObsId(const std::string & name) const {
    std::map<std::string, ObsId>::const_iterator it = name_to_oid.find(name);
    if (it == name_to_oid.end()) {
        stringstream ss;
        ss << __FUNCTION__ << ": did not find variable '" << name << "'";
        throw NotFoundException(ss.str());
    }
    return it->second;
}

double VarIdManager::get_default(const ParId & id) const{
    std::map<ParId, double>::const_iterator it = pid_to_default.find(id);
    if (it == pid_to_default.end()) {
        throw NotFoundException("VarIdManager::getDefault: did not find given ParId.");
    }
    return it->second;
}

const pair<double, double> & VarIdManager::get_range(const ParId & id) const{
    std::map<ParId, pair<double, double> >::const_iterator it = pid_to_range.find(id);
    if (it == pid_to_range.end()) {
        throw NotFoundException("VarIdManager::getRange: did not find given variable id.");
    }
    return it->second;
}

size_t VarIdManager::get_nbins(const ObsId & id) const{
    std::map<ObsId, size_t>::const_iterator it = oid_to_nbins.find(id);
    if (it == oid_to_nbins.end()) {
        throw NotFoundException("VarIdManager::getNbins: did not find given variable id.");
    }
    return it->second;
}

const pair<double, double> & VarIdManager::get_range(const ObsId & id) const{
    std::map<ObsId, pair<double, double> >::const_iterator it = oid_to_range.find(id);
    if (it == oid_to_range.end()) {
        throw NotFoundException("VarIdManager::getRange: did not find given variable id.");
    }
    return it->second;
}

/* ParValues */
ParIds ParValues::getAllParIds() const {
    ParIds result;
    for (size_t i=0; i<values.size(); i++) {
        if(not isnan(values[i]))
            result.insert(ParId(i));
    }    
    return result;
}

