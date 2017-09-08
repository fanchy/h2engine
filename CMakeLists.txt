cmake_minimum_required(VERSION 2.8)
project(h2engine)
include_directories(${PROJECT_SOURCE_DIR}/fflib ${PROJECT_SOURCE_DIR}/lib3party ${PROJECT_SOURCE_DIR}/src  
            ${PROJECT_SOURCE_DIR}/lib3party/php ${PROJECT_SOURCE_DIR}/lib3party/php/embed 
            ${PROJECT_SOURCE_DIR}/lib3party/php/main ${PROJECT_SOURCE_DIR}/lib3party/php/TSRM
            /usr/include/mysql/ /usr/include/python2.7/ /usr/include/python2.6/ 
)
link_directories(/usr/local/lib /usr/lib64/mysql)

set(ENGINE_LIB_SRC 
    ${PROJECT_SOURCE_DIR}/fflib/db/sqlite3.c
    ${PROJECT_SOURCE_DIR}/fflib/db/sqlite_ops.cpp
    ${PROJECT_SOURCE_DIR}/fflib/db/ffdb.cpp
    ${PROJECT_SOURCE_DIR}/fflib/db/mysql_ops.cpp
    ${PROJECT_SOURCE_DIR}/fflib/net/socketimpl/socket_win.cpp
    ${PROJECT_SOURCE_DIR}/fflib/net/socketimpl/socket_linux.cpp
    ${PROJECT_SOURCE_DIR}/fflib/net/eventloopimpl/epoll.cpp
    ${PROJECT_SOURCE_DIR}/fflib/net/eventloopimpl/select.cpp
    ${PROJECT_SOURCE_DIR}/fflib/net/ctrlimpl/socket_ctrl_gate.cpp
    ${PROJECT_SOURCE_DIR}/fflib/net/ctrlimpl/socket_ctrl_common.cpp
    ${PROJECT_SOURCE_DIR}/fflib/net/acceptorimpl/acceptor_linux_gate.cpp
    ${PROJECT_SOURCE_DIR}/fflib/net/acceptorimpl/acceptor_linux.cpp
    ${PROJECT_SOURCE_DIR}/fflib/net/codec.cpp
    ${PROJECT_SOURCE_DIR}/fflib/net/net_stat.cpp
    ${PROJECT_SOURCE_DIR}/fflib/base/lock.cpp
    ${PROJECT_SOURCE_DIR}/fflib/base/event_bus.cpp
    ${PROJECT_SOURCE_DIR}/fflib/base/thread.cpp
    ${PROJECT_SOURCE_DIR}/fflib/base/log.cpp
    ${PROJECT_SOURCE_DIR}/fflib/base/performance_daemon.cpp
    ${PROJECT_SOURCE_DIR}/fflib/rpc/ffrpc.cpp
    ${PROJECT_SOURCE_DIR}/fflib/rpc/ffbroker.cpp
    ${PROJECT_SOURCE_DIR}/fflib/rpc/msg_def/ffrpc_msg_types.cpp
    ${PROJECT_SOURCE_DIR}/fflib/rpc/msg_def/ffrpc_msg_constants.cpp
    ${PROJECT_SOURCE_DIR}/fflib/server/gen-cpp/msg_constants.cpp
    ${PROJECT_SOURCE_DIR}/fflib/server/gen-cpp/msg_types.cpp
    ${PROJECT_SOURCE_DIR}/fflib/server/ffgate.cpp
    ${PROJECT_SOURCE_DIR}/fflib/server/http_mgr.cpp
    ${PROJECT_SOURCE_DIR}/fflib/server/db_mgr.cpp
    ${PROJECT_SOURCE_DIR}/fflib/server/ffworker.cpp
    ${PROJECT_SOURCE_DIR}/fflib/server/shared_mem.cpp
    ${PROJECT_SOURCE_DIR}/fflib/xml/tinystr.cpp
    ${PROJECT_SOURCE_DIR}/fflib/xml/tinyxml.cpp
    ${PROJECT_SOURCE_DIR}/fflib/xml/tinyxmlerror.cpp
    ${PROJECT_SOURCE_DIR}/fflib/xml/ffxml.cpp
    ${PROJECT_SOURCE_DIR}/fflib/xml/tinyxmlparser.cpp
)
set(GAME_EXAMPLE_SRC
    ${PROJECT_SOURCE_DIR}/src/game/entity.cpp
    ${PROJECT_SOURCE_DIR}/src/game/player_service.cpp
    ${PROJECT_SOURCE_DIR}/src/game/map.cpp
    ${PROJECT_SOURCE_DIR}/src/game/db_service.cpp
    ${PROJECT_SOURCE_DIR}/src/game/task.cpp
    ${PROJECT_SOURCE_DIR}/src/game/item.cpp
)
set(ENGINE_SRC_LIST 
    ${ENGINE_LIB_SRC}
    ${PROJECT_SOURCE_DIR}/src/engine/main.cpp
)
set(WORKER_PY_SRC_LIST 
    ${ENGINE_LIB_SRC} ${GAME_EXAMPLE_SRC}
    ${PROJECT_SOURCE_DIR}/src/workerpy/ffworker_python.cpp
    ${PROJECT_SOURCE_DIR}/src/workerpy/main.cpp
)
set(WORKER_LUA_SRC_LIST 
    ${ENGINE_LIB_SRC} ${GAME_EXAMPLE_SRC}
    ${PROJECT_SOURCE_DIR}/src/workerlua/ffworker_lua.cpp
    ${PROJECT_SOURCE_DIR}/src/workerlua/main.cpp
)
set(WORKER_JS_SRC_LIST 
    ${ENGINE_LIB_SRC} ${GAME_EXAMPLE_SRC}
    ${PROJECT_SOURCE_DIR}/src/workerjs/ffworker_js.cpp
    ${PROJECT_SOURCE_DIR}/src/workerjs/main.cpp
)
set(WORKER_PHP_SRC_LIST 
    ${ENGINE_LIB_SRC} ${GAME_EXAMPLE_SRC}
    ${PROJECT_SOURCE_DIR}/src/workerphp/ffworker_php.cpp
    ${PROJECT_SOURCE_DIR}/src/workerphp/main.cpp
)
add_executable(h2engine ${ENGINE_SRC_LIST})
target_link_libraries(h2engine pthread dl mysqlclient crypto ssl z curl)

add_executable(h2workerpy ${WORKER_PY_SRC_LIST})
target_link_libraries(h2workerpy pthread dl mysqlclient crypto ssl z curl python2.6)

add_executable(h2workerlua ${WORKER_LUA_SRC_LIST})
target_link_libraries(h2workerlua pthread dl mysqlclient crypto ssl z curl lua)

add_executable(h2workerjs ${WORKER_JS_SRC_LIST})
target_link_libraries(h2workerjs pthread dl mysqlclient crypto ssl z curl v8)

add_executable(h2workerphp ${WORKER_PHP_SRC_LIST})
target_link_libraries(h2workerphp pthread dl mysqlclient crypto ssl z curl php5)


