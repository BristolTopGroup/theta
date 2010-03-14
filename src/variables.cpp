#include "interface/variables.hpp"
#include "interface/variables-utils.hpp"
#include "interface/exception.hpp"
#include "interface/cfg-utils.hpp"

#include <sstream>
#include <cmath>

using namespace theta;
using namespace theta::utils;
using namespace std;
using namespace libconfig;

ParId VarIdManager::createParId(const std::string & name, double def, double min, double max) {
    if (parNameExists(name)) {
            stringstream ss;
            ss << "VarIdManager::createParId: parameter '"<< name <<"' defined twice";
            throw InvalidArgumentException(ss.str());
    }
    if (min > max) {
        stringstream ss;
        ss << "Parameter " << name << " has min > max, i.e., empty range";
        throw InvalidArgumentException(ss.str());
    }
    if (def < min || def > max) {
        stringstream ss;
        ss << "Parameter '" << name << "' has default value outside of its range";
        throw InvalidArgumentException(ss.str());
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
            stringstream ss;
            ss << "VarIdManager::createObsId: observable '" << name << "' defined twice";
            throw InvalidArgumentException(ss.str());
    }
    if (min >= max) {
         stringstream ss;
         ss << "Observable " << name << " has min >= max, i.e., empty range";
         throw InvalidArgumentException(ss.str());
    }
    if(nbins==0){
        stringstream ss;
        ss << "Observable '" << name << "' has no bins";
        throw InvalidArgumentException(ss.str());
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

void VarIdManager::set_range_default(const ParId & id, double low, double high, double def){
    std::map<ParId, double>::iterator it = pid_to_default.find(id);
    std::map<ParId, pair<double, double> >::iterator itt = pid_to_range.find(id);
    if (it == pid_to_default.end() || itt == pid_to_range.end()) {
        throw NotFoundException("VarIdManager::set_range_default: did not find given ParId.");
    }
    if(def < low || def > high){
       throw InvalidArgumentException("VarIdManager::set_range_default: default not included in range!");
    }
    it->second = def;
    itt->second.first = low;
    itt->second.second = high;
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

ParValues VarIdManager::get_defaults() const{
    ParValues result;
    for(std::map<ParId, double>::const_iterator it = pid_to_default.begin(); it!= pid_to_default.end(); ++it){
        result.set(it->first, it->second);
    }
    return result;
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

ObsIds VarIdManager::getAllObsIds() const{
    std::map<ObsId, pair<double, double> >::const_iterator it = oid_to_range.begin();
    ObsIds result;
    for(; it!= oid_to_range.end(); ++it){
       result.insert(it->first);
    }
    return result;
}

ParIds VarIdManager::getAllParIds() const{
    std::map<ParId, pair<double, double> >::const_iterator it = pid_to_range.begin();
    ParIds result;
    for(; it!= pid_to_range.end(); ++it){
       result.insert(it->first);
    }
    return result;
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

void theta::VarIdManagerUtils::apply_settings(theta::plugin::Configuration & ctx){
    SettingWrapper s = ctx.setting;
    size_t nobs = s["observables"].size();
    if (nobs == 0){
        stringstream ss;
        ss << "No observables defined in " << s["observables"].getPath();
        throw ConfigurationException(ss.str());
    }
    for (size_t i = 0; i < nobs; i++) {
        string obs_name = s["observables"][i].getName();
        double min = s["observables"][i]["range"][0].get_double_or_inf();
        double max = s["observables"][i]["range"][1].get_double_or_inf();
        unsigned int nbins = s["observables"][i]["nbins"];
        ctx.vm->createObsId(obs_name, (size_t) nbins, min, max);
    }
    //get parameters:
    size_t npar = s["parameters"].size();
    if (npar == 0){
        stringstream ss;
        ss << "No parameters defined in " << s["parameters"].getPath();
        throw ConfigurationException(ss.str());
    }
    for (size_t i = 0; i < npar; i++) {
        string par_name = s["parameters"][i].getName();
        double def = s["parameters"][i]["default"];
        double min = s["parameters"][i]["range"][0].get_double_or_inf();
        double max = s["parameters"][i]["range"][1].get_double_or_inf();

        ctx.vm->createParId(par_name, def, min, max);
    }
}

