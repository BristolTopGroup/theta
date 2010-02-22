#include "interface/exception.hpp"
#include <iostream>

using namespace theta;

InvalidArgumentException::InvalidArgumentException(const std::string & m) : Exception(m) {}

Exception::Exception(const std::string & m):message(m){}

ConfigurationException::ConfigurationException(const std::string & msg): Exception(msg){}

NotFoundException::NotFoundException(const std::string & msg): Exception(msg){}

MathException::MathException(const std::string & m): Exception(m){}

