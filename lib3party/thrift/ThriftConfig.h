
#ifndef _THRIFT_THRIFT_CONFIG_H_
#define _THRIFT_THRIFT_CONFIG_H_ 1

#include <assert.h>

#define T_VIRTUAL_CALL()
#define  BOOST_STATIC_ASSERT(X) assert(X)
#define TDB_LIKELY(val) (val)

#include <string>
#include <map>
#include <list>
#include <set>
#include <vector>
#include <exception>
#include <typeinfo>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#include <Windows.h>
#include <Windows.h>
//#pragma comment(lib, "Ws2_32.lib")
#include <stdint.h>
/*
typedef __int64  int64_t;
typedef unsigned __int64  uint64_t;
typedef unsigned int uint32_t;
typedef int int32_t;
typedef short int16_t;
typedef unsigned short  uint16_t;
typedef char  int8_t;
typedef BYTE  uint8_t;
*/
#else
#include <stdint.h>
#endif


#endif
