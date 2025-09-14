cmake_minimum_required(VERSION 3.20)
project(GateServer)

set(CMAKE_TOOLCHAIN_FILE "/home/chenshouyang/vcpkg/scripts/buildsystems/vcpkg.cmake")

set(CMAKE_CXX_STANDARD 17)


set(CMAKE_PREFIX_PATH "/home/chenshouyang/vcpkg/installed/x64-linux" ${CMAKE_PREFIX_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
find_path(MYSQL_INCLUDE_DIR mysql.h
    PATHS /usr/include/mysql /usr/local/include/mysql
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
    ${MYSQL_INCLUDE_DIR}
)


find_package(Boost REQUIRED COMPONENTS system thread filesystem)
find_package(gRPC CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(redis++ CONFIG REQUIRED)


find_package(unofficial-mysql-connector-cpp CONFIG REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)



file(GLOB_RECURSE INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/includes/*.h)
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

file(GLOB_RECURSE PROTO_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.cc)

message(STATUS "Source files: ${SOURCES}")


add_executable(${PROJECT_NAME} ${INCLUDES} ${SOURCES} ${PROTO_SOURCE})



target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::system
    Boost::thread
    Boost::filesystem
    gRPC::grpc++
    gRPC::grpc
    gRPC::gpr
    protobuf::libprotoc
    protobuf::libprotobuf
    protobuf::libprotobuf-lite
    JsonCpp::JsonCpp
    redis++::redis++_static   
    unofficial::mysql-connector-cpp::connector
    mysqlpp
)


target_include_directories(${PROJECT_NAME} PRIVATE    ${Boost_INCLUDE_DIRS}
    ${gRPC_INCLUDE_DIRS}
    ${protobuf_INCLUDE_DIRS}
    ${jsoncpp_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
    ${redis++_INCLUDE_DIRS}
    ${MYSQL_INCLUDE_DIR} 
)
# 