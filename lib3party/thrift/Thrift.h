
#ifndef _THRIFT_THRIFT_H_
#define _THRIFT_THRIFT_H_ 1

#include <time.h>
#include <string.h>
#include <cstring>
#include <stdio.h>
#ifndef WIN32
#include <arpa/inet.h>
#include <thrift/ThriftConfig.h>
#else
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include "./ThriftConfig.h"
#endif
#define THRIFT_UNUSED_VARIABLE(x)  
namespace apache { namespace thrift {

class TEnumIterator
	: public std::iterator<std::forward_iterator_tag, std::pair<int, const char*> > {
public:
	TEnumIterator(int n, int* enums, const char** names)
		: ii_(0), n_(n), enums_(enums), names_(names) {}

	int operator++() { return ++ii_; }

	bool operator!=(const TEnumIterator& end) {
		return (ii_ != n_);
	}
	bool operator==(const TEnumIterator& end) {
		return (ii_ == n_);
	}
	std::pair<int, const char*> operator*() const { return std::make_pair(enums_[ii_], names_[ii_]); }

private:
	int ii_;
	const int n_;
	int* enums_;
	const char** names_;
};

class TOutput {
public:
    TOutput() : f_(&errorTimeWrapper) {}

    inline void setOutputFunction(void (*function)(const char *)){
        f_ = function;
    }

    inline void operator()(const char *message){
        f_(message);
    }

    // It is important to have a const char* overload here instead of
    // just the string version, otherwise errno could be corrupted
    // if there is some problem allocating memory when constructing
    // the string.
    void perror(const char *message, int errno_copy);
    inline void perror(const std::string &message, int errno_copy) {
        perror(message.c_str(), errno_copy);
    }

    void printf(const char *message, ...);

    inline static void errorTimeWrapper(const char* msg) {
        time_t now;
        char dbgtime[26];
        time(&now);
        //ctime_r(&now, dbgtime);
#ifdef __linux__
        char* p = ctime(&now);
        ::strcpy(dbgtime, p);
#else
		char* p = ctime(&now);
		::strcpy(dbgtime, p);
        //ctime_r(&now, dbgtime);//ctime_s(dbgtime, sizeof(dbgtime), &now);
#endif
        dbgtime[24] = 0;
        fprintf(stderr, "Thrift: %s %s\n", dbgtime, msg);
    }

    /** Just like strerror_r but returns a C++ string object. */
    static std::string strerror_s(int errno_copy);

private:
    void (*f_)(const char *);
};


class TException : public std::exception {
public:
    TException():
      message_() {}

      TException(const std::string& message) :
      message_(message) {}

      virtual ~TException() throw() {}

      virtual const char* what() const throw() {
          if (message_.empty()) {
              return "Default TException.";
          } else {
              return message_.c_str();
          }
      }

protected:
    std::string message_;

};

}}
#endif
