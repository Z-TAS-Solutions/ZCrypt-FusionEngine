cmake_minimum_required(VERSION 3.16)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/build/Release/generators"
                      "${CMAKE_SOURCE_DIR}/build/generators")

project(ZCrypt-FusionEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(gRPC CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)

set(GEN_DIR "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GEN_DIR})

set(PROTO_SRC "${GEN_DIR}/zfusion.pb.cc")
set(PROTO_HDR "${GEN_DIR}/zfusion.pb.h")
set(GRPC_SRC "${GEN_DIR}/zfusion.grpc.pb.cc")
set(GRPC_HDR "${GEN_DIR}/zfusion.grpc.pb.h")
set(PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/proto/zfusion.proto")

add_custom_command(
  OUTPUT ${PROTO_SRC} ${PROTO_HDR} ${GRPC_SRC} ${GRPC_HDR}
  COMMAND
    $<TARGET_FILE:protobuf::protoc> --cpp_out=${GEN_DIR} --grpc_out=${GEN_DIR}
    --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin> -I
    ${CMAKE_CURRENT_SOURCE_DIR}/src/proto ${PROTO_FILES}
  DEPENDS ${PROTO_FILES}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  VERBATIM)

add_custom_target(proto_gen ALL DEPENDS ${PROTO_SRC} ${PROTO_HDR} ${GRPC_SRC}
                                        ${GRPC_HDR})

# Fetch custom OpenCV with GStreamer support
include(FetchContent)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../opencv-4.9.0-gst-x64.zip")
  set(OPENCV_URL "${CMAKE_CURRENT_SOURCE_DIR}/../opencv-4.9.0-gst-x64.zip")
else()
  set(OPENCV_URL "https://github.com/Z-TAS-Solutions/ZScanner/releases/download/v0.1.0/opencv-4.9.0-gst-x64.zip")
endif()

FetchContent_Declare(
  opencv_custom
  URL "${OPENCV_URL}"
  URL_HASH
    SHA256=2E86023D6B71B8B9EEA0888A1896AF1FA91395FC819A0C0D7E99BA2A3101658D
  DOWNLOAD_EXTRACT_TIMESTAMP true)

FetchContent_MakeAvailable(opencv_custom)

set(APP_SOURCES src/main.cpp src/FusionServer.cpp src/HammingDistance.cpp
                src/CaptureEngine.cpp)

add_executable(
  ZCrypt-FusionEngine ${APP_SOURCES} ${PROTO_SRC} ${GRPC_SRC}
                      "src/ZFusionCore.cpp" "src/ZCore.cpp" "src/ZCore.h")
add_dependencies(ZCrypt-FusionEngine proto_gen)

target_compile_definitions(
  ZCrypt-FusionEngine PUBLIC $<$<CONFIG:Debug>:ZSCAN_BUILD_DEBUG>
                             $<$<CONFIG:Release>:ZSCAN_BUILD_RELEASE>)

add_custom_command(
  TARGET ZCrypt-FusionEngine
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy_directory
    "${opencv_custom_SOURCE_DIR}/dll/$<CONFIG>"
    "$<TARGET_FILE_DIR:ZCrypt-FusionEngine>"
  COMMENT "Copying OpenCV Runtime DLLs ($<CONFIG>) to build directory...")

target_include_directories(
  ZCrypt-FusionEngine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${GEN_DIR}
                              ${opencv_custom_SOURCE_DIR}/include)

target_link_libraries(
  ZCrypt-FusionEngine
  PRIVATE gRPC::grpc++
          protobuf::libprotobuf
          debug
          "${opencv_custom_SOURCE_DIR}/lib/Debug/opencv_world490d.lib"
          optimized
          "${opencv_custom_SOURCE_DIR}/lib/Release/opencv_world490.lib")

if(WIN32)
  target_link_libraries(ZCrypt-FusionEngine PRIVATE ws2_32)
endif()

# Automatic compile commands to root for nvim. The End.
if(CMAKE_EXPORT_COMPILE_COMMANDS)
  if(CMAKE_HOST_WIN32)
    file(
      WRITE "${CMAKE_SOURCE_DIR}/.clangd"
      "CompileFlags:
  CompilationDatabase: \"${CMAKE_BINARY_DIR}\"
")
  else()
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/compile_commands.json")
      execute_process(
        COMMAND
          ${CMAKE_COMMAND} -E create_symlink
          "${CMAKE_BINARY_DIR}/compile_commands.json"
          "${CMAKE_SOURCE_DIR}/compile_commands.json")
    endif()
  endif()
endif()
