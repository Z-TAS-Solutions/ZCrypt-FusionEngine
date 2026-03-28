cmake_minimum_required(VERSION 3.16)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/build/generators") 

project(ZCrypt-FusionEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(gRPC CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)

set(GEN_DIR "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GEN_DIR})

set(PROTO_SRC "${GEN_DIR}/zfusion.pb.cc")
set(PROTO_HDR "${GEN_DIR}/zfusion.pb.h")
set(GRPC_SRC  "${GEN_DIR}/zfusion.grpc.pb.cc")
set(GRPC_HDR  "${GEN_DIR}/zfusion.grpc.pb.h")
set(PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/proto/zfusion.proto")

add_custom_command(
    OUTPUT ${PROTO_SRC} ${PROTO_HDR} ${GRPC_SRC} ${GRPC_HDR}
    COMMAND $<TARGET_FILE:protobuf::protoc>
            --cpp_out=${GEN_DIR}
            --grpc_out=${GEN_DIR}
            --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
            -I ${CMAKE_CURRENT_SOURCE_DIR}/src/proto
            ${PROTO_FILES}
    DEPENDS ${PROTO_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
)

add_custom_target(proto_gen ALL DEPENDS ${PROTO_SRC} ${PROTO_HDR} ${GRPC_SRC} ${GRPC_HDR})

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH x64)
else()
    set(ARCH x86)
endif()

file(GLOB EXTERNAL_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/external/lib/Release/${ARCH}/*.lib")
file(GLOB EXTERNAL_ULIBS "${CMAKE_CURRENT_SOURCE_DIR}/external/lib/Universal/${ARCH}/*.lib")

set(APP_SOURCES
    src/main.cpp
    src/FusionServer.cpp
    src/HammingDistance.cpp
    src/CaptureEngine.cpp
)

add_executable(ZCrypt-FusionEngine ${APP_SOURCES} ${PROTO_SRC} ${GRPC_SRC})
add_dependencies(ZCrypt-FusionEngine proto_gen)

target_include_directories(ZCrypt-FusionEngine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GEN_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/external/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/include/opencv2
)

target_link_libraries(ZCrypt-FusionEngine PRIVATE
    gRPC::grpc++
    protobuf::libprotobuf
    ${EXTERNAL_ULIBS}
    ${EXTERNAL_LIBS}
)