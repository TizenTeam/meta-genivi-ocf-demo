#ifndef logger_h_
#define logger_h_ 1

#include <iostream>
#include <dlog.h>


class Log : public std::ostream
{
public:
  Log& operator<<(const std::string& str) {
    std::cout << str;
    dlog_print(DLOG_INFO, LOG_TAG, "%s", str.c_str() );

    return *this;
  }
  Log& operator<<(const char * str) {
    std::cout << str;
    dlog_print(DLOG_INFO, LOG_TAG, "%s", str );
    return *this;
  }
  Log& operator<<(const char str) {
    std::cout << str;
    //TRACEfs( str );
    return *this;
  }

  // http://stackoverflow.com/questions/1134388/stdendl-is-of-unknown-type-when-overloading-operator
  Log &operator<<(std::ostream& (*os)(std::ostream&)){
    std::cout << *os;
    return *this;
  }

  Log& operator<<(const int str) {
    std::cout << str;
    //TRACEfs( str );
    return *this;
  }
  friend std::ostream& operator<<(std::ostream& os, const Log& stream);

};

//std::ostream& operator<<(std::ostream& os, const Log& stream) {   return os; }

namespace std {
namespace local {
extern Log cout;
}
}
#endif
