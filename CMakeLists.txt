cmake_minimum_required(VERSION 3.22)

project(PtokaX VERSION 0.5.354 LANGUAGES C CXX)

# Export compile_commands.json for IDE/clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ccache support
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()

# Fast linker: prefer mold > lld > default
find_program(MOLD_PROGRAM mold)
if(MOLD_PROGRAM)
    add_link_options(-fuse-ld=mold)
    message(STATUS "Using mold linker")
else()
    find_program(LLD_PROGRAM ld.lld)
    if(LLD_PROGRAM)
        add_link_options(-fuse-ld=lld)
        message(STATUS "Using lld linker")
    endif()
endif()

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# AddressSanitizer support
option(ENABLE_ASAN "Enable AddressSanitizer" ON)
if(ENABLE_ASAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
    message(STATUS "AddressSanitizer: ENABLED")
else()
    message(STATUS "AddressSanitizer: disabled (only in Debug builds)")
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Git build number (auto-incremented per commit)
find_program(GIT_PROGRAM git)
if(GIT_PROGRAM)
    execute_process(
        COMMAND ${GIT_PROGRAM} rev-list --count HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BUILD_NUMBER
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()
if(NOT GIT_BUILD_NUMBER OR GIT_BUILD_NUMBER STREQUAL "")
    # Fallback: read from build_number.txt (generated before Docker build)
    if(EXISTS "${CMAKE_SOURCE_DIR}/build_number.txt")
        file(READ "${CMAKE_SOURCE_DIR}/build_number.txt" GIT_BUILD_NUMBER)
        string(STRIP "${GIT_BUILD_NUMBER}" GIT_BUILD_NUMBER)
    else()
        set(GIT_BUILD_NUMBER 0)
    endif()
endif()
message(STATUS "Build number (git): ${GIT_BUILD_NUMBER}")

# clang-tidy integration: runs as separate step (not during build)
# because system spdlog 1.15 + fmt 10.1 have consteval incompatibility with clang in C++20 mode

# Unity build (batch compilation for faster builds)
set(CMAKE_UNITY_BUILD ON)
set(CMAKE_UNITY_BUILD_BATCH_SIZE 0)

# Compiler-specific optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -fno-omit-frame-pointer -fsanitize=address,undefined")
    set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,undefined")
    # _GLIBCXX_ASSERTIONS: cheap runtime checks that invalidate iterator/container
    # misuse (e.g. incrementing an iterator after the element was erased). Does NOT
    # change ABI, so it is safe to mix with prebuilt external libs. (Do NOT use
    # _GLIBCXX_DEBUG here — it changes ABI and breaks linking against system libs.)
    add_compile_options(-D_GLIBCXX_ASSERTIONS)
elseif(CMAKE_BUILD_TYPE STREQUAL "Profile")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g -pg -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
    # Security hardening for network-facing server
    add_compile_options(-fstack-protector-strong -D_FORTIFY_SOURCE=2 -fstack-clash-protection -fPIE)
    add_link_options(-Wl,-z,relro,-z,now -pie)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2 -g -DNDEBUG")
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os -DNDEBUG")
endif()

# Enable exports and dynamic linking
set(CMAKE_ENABLE_EXPORTS ON)
add_link_options(-rdynamic)

# Project definitions
add_definitions(
    -D_WITH_SQLITE
    -DFLYLINKDC_USE_DB
    -DUSE_FLYLINKDC_EXT_JSON
    -DFLYLINKDC_USE_CPU_STAT
    -D_GNU_SOURCE
    -D_POSIX_C_SOURCE=200809L
    -D_DEFAULT_SOURCE
)

# Enable testing
enable_testing()

# Find system packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(LUA54 REQUIRED lua5.4)
# Prefer zlib-ng (SIMD-optimized) over system zlib.
# zlib-ng with ZLIB_COMPAT=ON installs as zlib.pc, which is API-identical.
# Check for zlib-ng first; fall back to system zlib.
pkg_check_modules(ZLIB_QUERY zlib)
execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --modversion zlib
    OUTPUT_VARIABLE ZLIB_RAW_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
string(FIND "${ZLIB_RAW_VERSION}" "zlib-ng" ZLIB_IS_ZLIB_NG)
if(${ZLIB_IS_ZLIB_NG} GREATER -1)
    set(ZLIB_IMPL "zlib-ng")
    execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=libdir zlib
        OUTPUT_VARIABLE ZLIB_LIBDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=includedir zlib
        OUTPUT_VARIABLE ZLIB_INCDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(ZLIB_LIBRARIES "${ZLIB_LIBDIR}/libz.so")
    set(ZLIB_NG_INCLUDE_DIRS "${ZLIB_INCDIR}")
    set(ZLIB_CFLAGS_OTHER "")
    # RPATH → zlib-ng first (system libz.so.1 has same soname but less performance)
    set(CMAKE_BUILD_RPATH "${ZLIB_LIBDIR}")
    set(CMAKE_INSTALL_RPATH "${ZLIB_LIBDIR}")
    message(STATUS "  zlib-ng include: ${ZLIB_NG_INCLUDE_DIRS}")
    message(STATUS "  zlib-ng library: ${ZLIB_LIBDIR}/libz.so")
else()
    # Use system zlib via pkg-config
    pkg_check_modules(ZLIB REQUIRED zlib)
    set(ZLIB_IMPL "zlib")
    message(STATUS "  Using system zlib: ${ZLIB_VERSION}")
endif()
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
pkg_check_modules(CIVETWEB REQUIRED civetweb)
pkg_check_modules(CIVETWEB_CPP REQUIRED civetweb-cpp)
pkg_check_modules(TINYXML2 REQUIRED tinyxml2)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(prometheus-cpp REQUIRED COMPONENTS core pull)

# Source files organization
set(PTOKAX_CORE_SOURCES
    core/colUsers.cpp
    core/DB-SQLite.cpp
    core/DcCommands.cpp
    core/DeFlood.cpp
    core/eventqueue.cpp
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
    core/Log.cpp
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
    core/ProfileManager.cpp
    core/PtokaX-nix.cpp
    core/PXBReader.cpp
    core/ResNickManager.cpp
    core/ServerManager.cpp
    core/ServerThread.cpp
    core/serviceLoop.cpp
    core/SettingManager.cpp
    core/TextConverter.cpp
    core/TextFileManager.cpp
    core/UdpDebug.cpp
    core/UDPThread.cpp
    core/User.cpp
    core/utility.cpp
    core/ZlibUtility.cpp
)

set(PTOKAX_EXTERNAL_SOURCES
    # Skein hash implementation
    skein/Optimized_32bit/skein_block.c
    skein/Optimized_32bit/skein.c

    # Test port
    fly-server-test-port/CDBManager.cpp
    fly-server-test-port/fly-server-test-port.cpp
)

# Create the main executable
add_executable(${PROJECT_NAME}
    ${PTOKAX_CORE_SOURCES}
    ${PTOKAX_EXTERNAL_SOURCES}
)

# Suppress warnings for vendored C code
set_source_files_properties(
    skein/Optimized_32bit/skein_block.c
    skein/Optimized_32bit/skein.c
    PROPERTIES COMPILE_OPTIONS "-w"
)

# Lua files have conflicting static function names - skip unity for them
set_source_files_properties(
    core/LuaBanManLib.cpp
    core/LuaCoreLib.cpp
    core/LuaIP2CountryLib.cpp
    core/LuaProfManLib.cpp
    core/LuaRegManLib.cpp
    core/LuaScriptManLib.cpp
    core/LuaSetManLib.cpp
    core/LuaTmrManLib.cpp
    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    skein/Optimized_32bit
    fly-server-test-port
    core
    ${LUA54_INCLUDE_DIRS}
    ${ZLIB_NG_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
    ${CIVETWEB_INCLUDE_DIRS}
)

# Compiler warnings for project code
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic -Wshadow -pipe>
    $<$<COMPILE_LANGUAGE:C>:-Wall -Wextra -pipe>
)

target_compile_options(${PROJECT_NAME} PRIVATE
    ${LUA54_CFLAGS_OTHER}
    ${ZLIB_CFLAGS_OTHER}
    ${SQLITE3_CFLAGS_OTHER}
    ${CIVETWEB_CFLAGS_OTHER}
    ${TINYXML2_CFLAGS_OTHER}
)

# Link libraries
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Threads::Threads
    ${CMAKE_DL_LIBS}
    m
    ${LUA54_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    ${CIVETWEB_LIBRARIES}
    ${CIVETWEB_CPP_LIBRARIES}
    ${TINYXML2_LIBRARIES}
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    prometheus-cpp::pull
    prometheus-cpp::core
)

# Set output name
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "PtokaX")

# Generate version header
configure_file(
    ${CMAKE_SOURCE_DIR}/core/version.h.in
    ${CMAKE_BINARY_DIR}/generated/version.h
    @ONLY
)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated)

# Installation rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# PVS-Studio static analysis integration
option(ENABLE_PVS_STUDIO "Enable PVS-Studio static analysis" OFF)
if(ENABLE_PVS_STUDIO)
    include(FetchContent)
    FetchContent_Declare(
        PVS_CMakeModule
        GIT_REPOSITORY "https://github.com/viva64/pvs-studio-cmake-module.git"
        GIT_TAG        "master"
    )
    FetchContent_MakeAvailable(PVS_CMakeModule)
    include("${pvs_cmakemodule_SOURCE_DIR}/PVS-Studio.cmake")

    pvs_studio_add_target(TARGET PtokaX.analyze
                          OUTPUT FORMAT json
                          ANALYZE PtokaX
                          MODE GA:1,2,3
                          LOG ${CMAKE_BINARY_DIR}/PVS-Studio.log
                           ARGS -e ${CMAKE_SOURCE_DIR}/sqlite
                                -e ${CMAKE_SOURCE_DIR}/skein)
    message(STATUS "  PVS-Studio: enabled (cmake --build . --target PtokaX.analyze)")
endif()

# Zlib benchmark (links against whichever zlib implementation was chosen)
add_executable(bench_zlib benches/bench_zlib.cpp)
target_compile_options(bench_zlib PRIVATE -Wall -Wextra)
target_include_directories(bench_zlib PRIVATE ${ZLIB_NG_INCLUDE_DIRS})
target_link_libraries(bench_zlib PRIVATE ${ZLIB_LIBRARIES})
add_test(NAME bench_zlib COMMAND bench_zlib)

# GoogleTest unit tests (skipped in Docker build where network is unavailable)
option(BUILD_TESTING "Build unit tests" ON)
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(test_utility tests/test_utility.cpp)
    target_compile_options(test_utility PRIVATE -Wall -Wextra)
    target_include_directories(test_utility PRIVATE core ${CMAKE_BINARY_DIR}/generated)
    target_link_libraries(test_utility PRIVATE GTest::gtest_main spdlog::spdlog)
    add_test(NAME test_utility COMMAND test_utility)
endif()

# Print summary
message(STATUS "PtokaX configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Lua: ${LUA54_VERSION}")
message(STATUS "  zlib impl: ${ZLIB_IMPL} (${ZLIB_VERSION})")
message(STATUS "  SQLite: ${SQLITE3_VERSION}")
message(STATUS "  CivetWeb: ${CIVETWEB_VERSION}")
message(STATUS "  TinyXML2: ${TINYXML2_VERSION}")
