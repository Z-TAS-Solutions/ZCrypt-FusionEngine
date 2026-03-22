cmake_minimum_required(VERSION 3.16)
project(ZCrypt-FusionEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES src/*.cpp)


include_directories(ZCrypt-FusionEngine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/include)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(architecture x64)
else()
    set(architecture x86)
endif()

add_executable(ZCrypt-FusionEngine ${SOURCES})

#file(GLOB EXTERNAL_DLIBS
#    "${CMAKE_CURRENT_SOURCE_DIR}/external/lib/Debug/${architecture}/*.lib"
#)

#file(GLOB EXTERNAL_LIBS
#    "${CMAKE_CURRENT_SOURCE_DIR}/external/lib/Release/${architecture}/*.lib"
#)

#file(GLOB EXTERNAL_ULIBS
#    "${CMAKE_CURRENT_SOURCE_DIR}/external/lib/Universal/${architecture}/*.lib"
#)


#target_link_libraries(ZCrypt-FusionEngine PRIVATE 
#    ${EXTERNAL_ULIBS}
#    debug ${EXTERNAL_DLIBS}
#    optimized ${EXTERNAL_LIBS}
#)

