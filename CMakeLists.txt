cmake_minimum_required(VERSION 3.9.0)
project(ReTA VERSION 0.6.0 LANGUAGES CXX)

include(CPack)
include(FetchContent)
include_directories(lib/include)
include_directories(include)

option(RESOURCE_ACCESS "Enable access to resource availability" OFF)
option(DEBUG "Enable debugging" OFF)

if (DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
    message(NOTICE "Debug build")
else ()
    set(CMAKE_BUILD_TYPE Release)
endif ()

if (RESOURCE_ACCESS)
    message(NOTICE "Resource access is enabled")
    add_compile_definitions(RESOURCE_ACCESS)
endif ()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

set(MAIN_SOURCES src/reta.cpp lib/src/OptionParser.cpp)

add_executable(reta ${MAIN_SOURCES})

target_link_libraries(reta PUBLIC ${CORE_LIBS})
target_compile_features(reta PUBLIC cxx_std_17)