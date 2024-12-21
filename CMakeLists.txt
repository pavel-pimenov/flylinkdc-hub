cmake_minimum_required(VERSION 3.22)

project(PtokaX)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
set (CMAKE_ENABLE_EXPORTS true)
ADD_LINK_OPTIONS(-rdynamic)

add_definitions(-D_WITH_SQLITE)
#add_definitions(-DFLYLINKDC_USE_DB) // TODO error build MacOS
add_definitions(-DUSE_FLYLINKDC_EXT_JSON)
add_definitions(-DTIXML_USE_STL)

add_executable(PtokaX
 skein/Optimized_32bit/skein_block.c
 skein/Optimized_32bit/skein.c
 tinyxml/tinyxmlparser.cpp
 tinyxml/tinyxmlerror.cpp
 tinyxml/tinyxml.cpp
 core/colUsers.cpp
 core/DB-SQLite.cpp
 core/DcCommands.cpp
 core/DeFlood.cpp
 core/eventqueue.cpp
 core/ExceptionHandling.cpp
 core/GlobalDataQueue.cpp
 core/hashBanManager.cpp
 core/hashRegManager.cpp
 core/hashUsrManager.cpp
 core/HubCommands.cpp
 core/HubCommands-AE.cpp
 core/HubCommands-FH.cpp
 core/HubCommands-IQ.cpp
 core/HubCommands-RZ.cpp
 core/IP2Country.cpp
 core/LanguageManager.cpp
 core/LuaBanManLib.cpp
 core/LuaCoreLib.cpp
 core/LuaIP2CountryLib.cpp
 core/LuaProfManLib.cpp
 core/LuaRegManLib.cpp
 core/LuaScript.cpp
 core/LuaScriptManager.cpp
 core/LuaScriptManLib.cpp
 core/LuaSetManLib.cpp
 core/LuaTmrManLib.cpp
 core/LuaUDPDbgLib.cpp
 core/ProfileManager.cpp
 core/PtokaX-nix.cpp
 core/PXBReader.cpp
 core/pxstring.cpp
 core/RegThread.cpp
 core/ResNickManager.cpp
 core/ServerManager.cpp
 core/ServerThread.cpp
 core/serviceLoop.cpp
 core/SettingManager.cpp
 core/TextConverter.cpp
 core/TextFileManager.cpp
 core/UdpDebug.cpp
 core/UDPThread.cpp
 core/UpdateCheckThread.cpp
 core/User.cpp
 core/utility.cpp
 core/ZlibUtility.cpp
 jsoncpp/jsoncpp.cpp
 fly-server-test-port/CDBManager.cpp
 fly-server-test-port/fly-server-test-port.cpp
 sqlite/sqlite3x_command.cpp
 sqlite/sqlite3x_connection.cpp
 sqlite/sqlite3x_reader.cpp
 sqlite/sqlite3x_transaction.cpp
 sqlite/sqlite3.c
 prometheus-cpp/pull/src/basic_auth.cc
 prometheus-cpp/pull/src/endpoint.cc
 prometheus-cpp/pull/src/exposer.cc
 prometheus-cpp/pull/src/handler.cc
 prometheus-cpp/pull/src/metrics_collector.cc
 prometheus-cpp/core/src/check_names.cc
 prometheus-cpp/core/src/counter.cc
 prometheus-cpp/core/src/family.cc
 prometheus-cpp/core/src/gauge.cc
 prometheus-cpp/core/src/histogram.cc
 prometheus-cpp/core/src/info.cc
 prometheus-cpp/core/src/registry.cc
 prometheus-cpp/core/src/serializer.cc
 prometheus-cpp/core/src/summary.cc
 prometheus-cpp/core/src/text_serializer.cc
 prometheus-cpp/core/src/detail/builder.cc
 prometheus-cpp/core/src/detail/ckms_quantiles.cc
 prometheus-cpp/core/src/detail/time_window_quantiles.cc
 prometheus-cpp/core/src/detail/utils.cc
 civetweb/CivetServer.cpp
 civetweb/civetweb.c
)

#find_package(Boost REQUIRED COMPONENTS iostreams)
find_package(Lua REQUIRED)
find_package(ZLIB REQUIRED)

include_directories(${LUA_INCLUDE_DIR})
include_directories(prometheus-cpp/core/include)
include_directories(prometheus-cpp/include)
include_directories(prometheus-cpp/util/include)
include_directories(prometheus-cpp/pull/include)
include_directories(civetweb)
include_directories(jsoncpp)
include_directories(sqlite)
include_directories(skein/Optimized_32bit)
include_directories(tinyxml)
include_directories(fly-server-test-port)

target_link_libraries(${PROJECT_NAME} ${ZLIB_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${LUA_LIBRARIES})


