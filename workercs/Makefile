#交叉编译器路径
CROSS=
CP=/bin/cp
RM=-/bin/rm -rf
LN=/bin/ln -s
CFLAGS=
LDFLAGS=
#链接库名
LIB_NAME=
#链接库版本
LIB_VER=1.0.0
#平台
ARCH=
# 二进制目标
BIN=workercs

#源文件目录
SrcDir= . fflib fflib/RpcMsg thrift thrift/Collections thrift/Net35 thrift/Properties thrift/Protocol thrift/Server thrift/Transport Google.Protobuf Google.Protobuf/Collections Google.Protobuf/Compatibility Google.Protobuf/Properties Google.Protobuf/Reflection Google.Protobuf/WellKnownTypes 
#头文件目录
IncDir= ./
#连接库目录
LibDir= /usr/local/lib
SRCS=$(foreach dir,$(SrcDir),$(wildcard $(dir)/*.cs))
#INCS=$(foreach dir,$(IncDir),$(wildcard $(dir)/*.h))
INCS=$(foreach dir,$(IncDir),$(addprefix -I,$(dir)))
LINKS=$(foreach dir,$(LibDir),$(addprefix -L,$(dir)))
CFLAGS := $(CFLAGS) $(INCS) 
CSFLAGS := 
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    CSFLAGS += /define:linux
endif
LDFLAGS:= $(LINKS) $(LDFLAGS)
CC=csc
ARCH=PC
OBJS = $(SRCS:%.cpp=%.o)
.PHONY:all clean

all:$(BIN)
$(BIN):$(SRCS)
	$(CC) $(CSFLAGS) /out:$@ $(SRCS)
	@echo " OK!\tComplie $@ "
# @$(LN) $(shell pwd)/$(LIB_NAME).$(LIB_VER) /lib/$(LIB_NAME)

.PHONY: clean
clean:
	@echo "[$(ARCH)] \tCleaning files..."
	@$(RM) $(BIN)
