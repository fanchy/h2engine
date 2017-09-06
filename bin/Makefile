#交叉编译器路径
CROSS=
CP=/bin/cp
RM=-/bin/rm -rf
LN=/bin/ln -s 
#是否开启sqlite的支持，如需要关闭，设置为空字符串
FF_ENABLE_SQLITE_FLAG =#-DFF_ENABLE_SQLITE#-DFF_ENABLE_CURL
#-DFF_ENABLE_CURL
CFLAGS=-g -Wall -DFF_ENABLE_SQLITE -DFF_ENABLE_CURL
LDFLAGS= -O2 -lpthread  -ldl -lmysqlclient  -lcrypto -lssl -lz -lcurl
#-lcurl
#-llua 
#链接库名
LIB_NAME=
#链接库版本
LIB_VER=0.5.0
#平台
ARCH=
# 二进制目标
BIN=h2engine

#头文件目录
IncDir=  ./ ../fflib/  ../lib3party /usr/include/mysql/ /usr/include/python2.7/ /usr/include/python2.6/ ../lib3party/php ../lib3party/php/embed ../lib3party/php/main  ../lib3party/php/Zend ../lib3party/php/TSRM
#连接库目录
LibDir= /usr/local/lib /usr/lib64/mysql ./
#源文件目录
SrcDir= ./engine ../fflib/base ../fflib/net ../fflib/net/socketimpl ../fflib/net/ctrlimpl ../fflib/net/eventloopimpl ../fflib/net/acceptorimpl ../fflib/rpc  ../fflib/rpc/msg_def  ../fflib/db ../fflib/xml ../fflib/server ../fflib/server/gen-cpp 
SrcDirPY= ./game ./workerpy ../fflib/base ../fflib/net ../fflib/net/socketimpl ../fflib/net/ctrlimpl ../fflib/net/eventloopimpl ../fflib/net/acceptorimpl ../fflib/rpc  ../fflib/rpc/msg_def  ../fflib/db ../fflib/xml ../fflib/server ../fflib/server/gen-cpp 
SrcDirLUA= ./workerlua ../fflib/base ../fflib/net ../fflib/net/socketimpl ../fflib/net/ctrlimpl ../fflib/net/eventloopimpl ../fflib/net/acceptorimpl ../fflib/rpc  ../fflib/rpc/msg_def  ../fflib/db ../fflib/xml ../fflib/server ../fflib/server/gen-cpp 
SrcDirJS= ./workerjs ../fflib/base ../fflib/net ../fflib/net/socketimpl ../fflib/net/ctrlimpl ../fflib/net/eventloopimpl ../fflib/net/acceptorimpl ../fflib/rpc  ../fflib/rpc/msg_def  ../fflib/db ../fflib/xml ../fflib/server ../fflib/server/gen-cpp 
SrcDirPHP= ./workerphp ../fflib/base ../fflib/net ../fflib/net/socketimpl ../fflib/net/ctrlimpl ../fflib/net/eventloopimpl ../fflib/net/acceptorimpl ../fflib/rpc  ../fflib/rpc/msg_def  ../fflib/db ../fflib/xml ../fflib/server ../fflib/server/gen-cpp 
SRCS=$(foreach dir,$(SrcDir),$(wildcard $(dir)/*.cpp))
SRCSPY=$(foreach dir,$(SrcDirPY),$(wildcard $(dir)/*.cpp))
SRCSLUA=$(foreach dir,$(SrcDirLUA),$(wildcard $(dir)/*.cpp))
SRCSJS=$(foreach dir,$(SrcDirJS),$(wildcard $(dir)/*.cpp))
SRCSPHP=$(foreach dir,$(SrcDirPHP),$(wildcard $(dir)/*.cpp))
#INCS=$(foreach dir,$(IncDir),$(wildcard $(dir)/*.h))
INCS=$(foreach dir,$(IncDir),$(addprefix -I,$(dir)))
LINKS=$(foreach dir,$(LibDir),$(addprefix -L,$(dir)))
CFLAGS := $(CFLAGS) $(INCS)
LDFLAGS:= $(LINKS) $(LDFLAGS) 
CC=gcc
ARCH=PC
OBJS = $(SRCS:%.cpp=%.o)
OBJSPY = $(SRCSPY:%.cpp=%.o)
OBJSLUA = $(SRCSLUA:%.cpp=%.o)
OBJSJS = $(SRCSJS:%.cpp=%.o)
OBJSPHP = $(SRCSPHP:%.cpp=%.o)
.PHONY:all clean

all:
	@echo usage:make h2engine      build h2engine app
	@echo usage:make h2workerpy    build h2workerpy app
	@echo usage:make h2workerlua   build h2workerlua app
	@echo usage:make h2workerjs    build h2workerjs app
	@echo usage:make h2workerphp   build h2workerphp app
allbin:
	make h2engine
	make h2workerpy
	make h2workerlua
	make h2workerjs
	make h2workerphp
sqlite3.o:
	gcc -c ../fflib/db/sqlite3.c -o sqlite3.o
h2engine:$(OBJS) sqlite3.o
	g++ -o $@ $(OBJS) sqlite3.o $(LDFLAGS)
	@echo  "build $@ ok"
h2workerpy:$(OBJSPY) sqlite3.o
	g++ -o $@ $(OBJSPY) sqlite3.o $(LDFLAGS) -lpython2.6
	@echo  "build $@ ok"
h2workerlua:$(OBJSLUA) sqlite3.o
	g++ -o $@ $(OBJSLUA) sqlite3.o $(LDFLAGS) -llua
	@echo  "build $@ ok"
h2workerjs:$(OBJSJS) sqlite3.o
	g++ -o $@ $(OBJSJS) sqlite3.o $(LDFLAGS) -lv8
	@echo  "build $@ ok"
h2workerphp:$(OBJSPHP) sqlite3.o
	g++ -o $@ $(OBJSPHP) sqlite3.o -lphp5 $(LDFLAGS) 
	@echo  "build $@ ok"
%.o:%.cpp
	@echo  "Build $@..."
	@$(CC) $(CFLAGS)  -c $< -o $@

.PHONY: clean
clean:
	@echo  "[$(ARCH)] Cleaning files..."
	@$(RM) $(OBJS) $(OBJSPY) $(OBJSLUA) $(OBJSJS) $(OBJSPHP) h2worker* h2engine
