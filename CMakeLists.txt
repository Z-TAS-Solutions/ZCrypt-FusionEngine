cmake_minimum_required(VERSION 3.16)
project(ZCrypt-FusionEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(gRPC REQUIRED)
find_package(protobuf REQUIRED)

set(PROTOC_EXE $<TARGET_FILE:protobuf::protoc>)
set(GRPC_CPP_PLUGIN_EXE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GEN_DIR})

set(PROTO_SRC "${GEN_DIR}/zfusion.pb.cc")
set(PROTO_HDR "${GEN_DIR}/zfusion.pb.h")
set(GRPC_SRC  "${GEN_DIR}/zfusion.grpc.pb.cc")
set(GRPC_HDR  "${GEN_DIR}/zfusion.grpc.pb.h")

add_custom_command(
    OUTPUT ${PROTO_SRC} ${PROTO_HDR} ${GRPC_SRC} ${GRPC_HDR}
    COMMAND ${PROTOC_EXE}
            --cpp_out=${GEN_DIR}
            --grpc_out=${GEN_DIR}
            --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXE}
            -I ${CMAKE_CURRENT_SOURCE_DIR}/src/proto
            ${CMAKE_CURRENT_SOURCE_DIR}/src/proto/zfusion.proto
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/proto/zfusion.proto
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
)

set(APP_SOURCES
    src/main.cpp
    src/FusionServer.cpp
    src/HammingDistance.cpp
    src/CaptureEngine.cpp
)

add_executable(ZCrypt-FusionEngine
    ${APP_SOURCES}
    ${PROTO_SRC} ${GRPC_SRC}
)

target_include_directories(ZCrypt-FusionEngine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GEN_DIR}
)

target_link_libraries(ZCrypt-FusionEngine PRIVATE
    gRPC::grpc++
    protobuf::libprotobuf
)