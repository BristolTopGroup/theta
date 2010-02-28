#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <string>
#include <sstream>

namespace theta {

/** \brief Base class for all Exceptions used in %theta
 */
class Exception: virtual public std::exception{
public:
    /// The human-readable, english message 
    std::string message;
    Exception(const std::string & msg);
    //Exception();
    virtual ~Exception() throw(){}
    virtual const char* what()throw(){
         std::stringstream ss;
         ss << std::exception::what() << ": " << message;
         whatstring = ss.str();
         return whatstring.c_str();
    }
protected:
    std::string whatstring;
};

/** \brief Thrown in case of a request for a non-existing member in a conatiner-like structure is made. 
 */
class NotFoundException: public Exception{
public:
   NotFoundException(const std::string & message);
};

/** \brief Thrown during configuration file processing.
 */
class ConfigurationException: public Exception{
public:
    ConfigurationException(const std::string & msg);
};

/** \brief Thrown if a method is called which depends on a particular state of object which is not fulfilled
 */  
class IllegalStateException: public Exception{
public:
    IllegalStateException(const std::string & msg=""): Exception(msg){}
};

class MathException: public Exception{
public:
    MathException(const std::string &);
};

/// \brief General exception to indicate that arguments passed to a function are invalid (=do not correspond to documentation)
class InvalidArgumentException: public Exception{
private:
    void print_message() const;
public:
    //InvalidArgumentException(const char* mes);
    InvalidArgumentException(const std::string &);
};

/// \brief Thrown in case of database errors.
class DatabaseException: public Exception{
public:
    DatabaseException(const std::string & s): Exception(s){}
};

/// \brief Thrown in case of minimization errors.
class MinimizationException: public Exception{
public:
    MinimizationException(const std::string & s): Exception(s){}
};

}

#endif
