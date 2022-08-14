cmake_minimum_required(VERSION 3.23)

set(CPM_DOWNLOAD_VERSION 0.35.1)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake")
    file(DOWNLOAD https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()

include(${CPM_DOWNLOAD_LOCATION})

project(shenron)

set(CMAKE_CXX_STANDARD 23)

CPMAddPackage(
        name entt
        GITHUB_REPOSITORY skypjack/entt
        VERSION 3.10.1
)

CPMAddPackage(
        NAME json
        GITHUB_REPOSITORY nlohmann/json
        VERSION 3.10.4
)

CPMAddPackage(
        NAME fmt
        GITHUB_REPOSITORY fmtlib/fmt
        GIT_TAG 8.1.1
)

CPMAddPackage(
        NAME cppccodec
        GITHUB_REPOSITORY tplgy/cppcodec
        VERSION 0.2
        OPTIONS "BUILD_TESTING OFF"
)

CPMAddPackage(
        NAME eff_random
        GITHUB_REPOSITORY effolkronium/random
        VERSION 1.4.0
        OPTIONS "Random_BuildTests OFF"
)

CPMAddPackage(
        NAME spdlog
        GITHUB_REPOSITORY gabime/spdlog
        VERSION 1.10.0
)

CPMAddPackage(
        NAME argh
        GITHUB_REPOSITORY adishavit/argh
        VERSION 1.3.1
        OPTIONS "BUILD_EXAMPLES OFF" "BUILD_TESTS OFF"
)


CPMAddPackage(
        NAME spatial
        GIT_REPOSITORY git://git.code.sf.net/p/spatial/code
        GIT_TAG master
)


CPMAddPackage(
        NAME asyncplusplus
        GITHUB_REPOSITORY Amanieu/asyncplusplus
        VERSION 1.1
)


find_package(Boost REQUIRED COMPONENTS program_options regex)


include_directories(PUBLIC
        include/
        ${asio_SOURCE_DIR}/asio/include
        ${entt_SOURCE_DIR}/single_include
        ${argh_SOURCE_DIR}
        ${BOOST_LIBRARY_INCLUDES}
        ${json_SOURCE_DIR}/single_include
        ${fmt_SOURCE_DIR}/include
        ${eff_random_SOURCE_DIR}/include
        ${sqlitepp_SOURCE_DIR}
        ${cppcodec_SOURCE_DIR}/cppcodec
        ${spdlog_SOURCE_DIR}/include
        ${argh_SOURCE_DIR}
        ${spatial_SOURCE_DIR}/src
        ${Async++_SOURCE_DIR}/include
        )

file(GLOB_RECURSE SHENRON_INCLUDE include/*.h)
file(GLOB_RECURSE SHENRON_SRC src/*.cpp)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})
#set(CMAKE_CXX_FLAGS "-fpermissive")
set(SPDLOG_FMT_EXTERNAL_HO ON)

add_library(shenron ${SHENRON_INCLUDE} ${SHENRON_SRC})
link_libraries(shenron ${Boost_LIBRARIES} ${Async++_BINARY_DIR}/libasync++.so)

add_executable(netmud apps/netmud.cpp)
