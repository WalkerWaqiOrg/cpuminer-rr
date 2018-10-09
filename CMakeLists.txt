cmake_minimum_required(VERSION 3.10)

SET(CMAKE_CXX_FLAGS_DEBUG "-Wall -g -ggdb ") # $ENV{CXXFLAGS} -O0
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

include("cmake/HunterGate.cmake")
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.23.16.tar.gz"
    SHA1 "dbe5b1c966414c4a055983c11d5251a2d8f2f16d"
    FILEPATH ${CMAKE_CURRENT_LIST_DIR}/config.cmake
)

project(cpuminer_rr) # CXX

set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")


#include_directories(${PROJECT_SOURCE_DIR}/libcaf_core)
#include_directories(${PROJECT_SOURCE_DIR}/libcaf_io)
#include_directories(${PROJECT_SOURCE_DIR}/libcaf_opencl)
#include_directories(${PROJECT_SOURCE_DIR}/libcaf_openssl)
#include_directories(${PROJECT_SOURCE_DIR}/libcaf_python)
#include_directories(${PROJECT_SOURCE_DIR}/libcaf_test)
#include_directories(${PROJECT_SOURCE_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR}/include)

set(CMAKE_CXX_STANDARD 11)

#set(CAF_LIBRARIES ${PROJECT_SOURCE_DIR}/lib/libcaf_core.dylib) # dylib
#set(CAF_LIBRARIES ${CAF_LIBRARIES} ${PROJECT_SOURCE_DIR}/lib/libcaf_io.dylib)

set(CAF_LIBRARIES caf_core_static)

hunter_add_package(CURL)
find_package(CURL CONFIG REQUIRED)

hunter_add_package(log4cplus)
find_package(log4cplus CONFIG REQUIRED)

hunter_add_package(jansson)
find_package(jansson CONFIG REQUIRED)

hunter_add_package(Boost COMPONENTS thread system)
find_package(Boost COMPONENTS thread system)

set(cpuminer_rr_SOURCES main.cpp src/stratum_actor.cpp src/user_params.cpp src/util.cpp src/miner_actors.cpp
    src/stratum_json.cpp src/rrhash.cpp)
add_executable(cpuminer_rr ${cpuminer_rr_SOURCES})

set(lib_cpuminer_rr_SOURCES cpuminer_lib.cpp src/stratum_actor.cpp src/user_params.cpp src/util.cpp src/miner_actors.cpp
    src/stratum_json.cpp src/rrhash.cpp)
add_library(_cpuminer_rr SHARED ${lib_cpuminer_rr_SOURCES})

target_link_libraries(cpuminer_rr PUBLIC CURL::libcurl jansson::jansson ${CAF_LIBRARIES} Boost::thread Boost::system)
target_link_libraries(_cpuminer_rr PUBLIC CURL::libcurl jansson::jansson ${CAF_LIBRARIES} Boost::thread Boost::system)

# ${CMAKE_DL_LIBS} ${CURL_LIBRARY} ${PTHREAD_LIBRARIES}
