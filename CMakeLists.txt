cmake_minimum_required(VERSION 2.8)
project(h2engine)
add_definitions(-g -O2 -Wall -Wno-unused-const-variable -static)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
	MESSAGE(STATUS "current platform: Linux ")
    set(EXT_LIBS 
        dl mysqlclient
    )
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
	MESSAGE(STATUS "current platform: Windows")
    set(EXT_LIBS 
        ws2_32 mysql
    )
    set(LUAVER "lua")
    set(PYVER "python24")
    set(EXT_INCLUDE
        ${PROJECT_SOURCE_DIR}/windependency/include
        C:/Python27/include
    )
    set(EXT_LINK
        ${PROJECT_SOURCE_DIR}/windependency/lib
        C:/Python27/libs
    )
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Darwin")
	MESSAGE(STATUS "current platform: MacOSX")
    set(LUAVER "lua")
    set(EXT_LIBS 
        dl mysqlclient
    )
    set(EXT_INCLUDE
        /usr/local/opt/openssl/include
        /usr/local/opt/mysql-client/include
	/usr/local/Cellar/lua/5.3.5_1/include/lua
    )
    set(EXT_LINK
        /usr/local/opt/mysql-client/lib
        /usr/local/opt/openssl/lib
	/usr/local/Cellar/lua/5.3.5_1/lib
    )
ELSE ()
	MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")
ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")

include_directories(${PROJECT_SOURCE_DIR}/fflib ${PROJECT_SOURCE_DIR}/lib3party ${PROJECT_SOURCE_DIR}/src  
            ${PROJECT_SOURCE_DIR}/lib3party/php ${PROJECT_SOURCE_DIR}/lib3party/php/embed 
            ${PROJECT_SOURCE_DIR}/lib3party/php/main ${PROJECT_SOURCE_DIR}/lib3party/php/Zend
            ${PROJECT_SOURCE_DIR}/lib3party/php/TSRM
            /usr/include/mysql/ /usr/include/python2.7/ /usr/include/python2.6/ 
            /usr/include/lua5.2/ /usr/include/lua5.1/ 
            ${EXT_INCLUDE}
)
link_directories(/usr/local/lib /usr/lib64/mysql ${PROJECT_SOURCE_DIR} ${EXT_LINK})

file(GLOB_RECURSE ENGINE_LIB_SRC "fflib/*.c*") 
list(REMOVE_ITEM ENGINE_LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/fflib/python/ffpython.cpp)

foreach(filename ${ENGINE_LIB_SRC})  
message(${filename})
endforeach()

file(GLOB_RECURSE GAME_IMPL_SRC "src/*.cpp") 
foreach(filename ${GAME_IMPL_SRC})  
message(${filename})
endforeach()
file(GLOB_RECURSE WORKER_CPP_SRC "workercpp/*.cpp") 
foreach(filename ${WORKER_CPP_SRC})  
message(${filename})
endforeach()
file(GLOB_RECURSE WORKER_PY_SRC "workerpy/*.cpp") 
foreach(filename ${WORKER_PY_SRC})  
message(${filename})
endforeach()
file(GLOB_RECURSE WORKER_LUA_SRC "workerlua/*.cpp") 
foreach(filename ${WORKER_LUA_SRC})  
message(${filename})
endforeach()
file(GLOB_RECURSE WORKER_JS_SRC "workerjs/*.cpp") 
foreach(filename ${WORKER_JS_SRC})  
message(${filename})
endforeach()
file(GLOB_RECURSE WORKER_PHP_SRC "workerphp/*.cpp") 
foreach(filename ${WORKER_PHP_SRC})  
message(${filename})
endforeach()


set(ENGINE_SRC_LIST 
    ${ENGINE_LIB_SRC}
    ${PROJECT_SOURCE_DIR}/engine/main.cpp
)
set(WORKER_CPP_SRC_LIST 
    ${ENGINE_LIB_SRC} ${GAME_IMPL_SRC} ${WORKER_CPP_SRC}
)
set(WORKER_PY_SRC_LIST 
    ${ENGINE_LIB_SRC} ${GAME_IMPL_SRC} ${WORKER_PY_SRC} ${CMAKE_CURRENT_SOURCE_DIR}/fflib/python/ffpython.cpp
)
set(WORKER_LUA_SRC_LIST 
    ${ENGINE_LIB_SRC} ${GAME_IMPL_SRC} ${WORKER_LUA_SRC}
)
set(WORKER_JS_SRC_LIST 
    ${ENGINE_LIB_SRC} ${GAME_IMPL_SRC} ${WORKER_JS_SRC}
)
set(WORKER_PHP_SRC_LIST 
    ${ENGINE_LIB_SRC} ${GAME_IMPL_SRC} ${WORKER_PHP_SRC}
)
add_executable(h2engine ${ENGINE_SRC_LIST})


target_link_libraries(h2engine pthread ${EXT_LIBS} crypto ssl z curl)

if(PYVER)
    message("user python version:" ${PYVER})
else()
    message("user default python version:python2.7")
    set(PYVER "python2.7") 
endif()
if(LUAVER)
    message("user lua version:" ${LUAVER})
else()
    message("user default lua version:lua5.2")
    set(LUAVER "lua5.2") 
endif()
if(PHPVER)
    message("user php version:" ${PHPVER})
else()
    message("user default php version:php5")
    set(PHPVER "php5") 
endif()

add_executable(h2workercpp ${WORKER_CPP_SRC_LIST})
target_link_libraries(h2workercpp pthread ${EXT_LIBS} crypto ssl z curl)

add_executable(h2workerpy ${WORKER_PY_SRC_LIST})
target_link_libraries(h2workerpy pthread ${EXT_LIBS} crypto ssl z curl ${PYVER})

add_executable(h2workerlua ${WORKER_LUA_SRC_LIST})
target_link_libraries(h2workerlua pthread ${EXT_LIBS} crypto ssl z curl ${LUAVER})

add_executable(h2workerjs ${WORKER_JS_SRC_LIST})
target_link_libraries(h2workerjs pthread ${EXT_LIBS} crypto ssl z curl v8)

add_executable(h2workerphp ${WORKER_PHP_SRC_LIST})
target_link_libraries(h2workerphp pthread ${EXT_LIBS} crypto ssl z curl ${PHPVER})

