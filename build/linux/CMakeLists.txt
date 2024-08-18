# ========================= cmake basic definition ========================= #
cmake_minimum_required(VERSION 3.10)
set(C_STANDARD 99)
if (${CORE} STREQUAL "LEAF")
    project(leafpython_linux)
elseif (${CORE} STREQUAL "PIKA")
    project(pikapython_linux)
endif()

# ================================ variables =============================== #
set(EXE_NAME "leafpython")

set(ROOT_DIR ${CMAKE_SOURCE_DIR}/../..)
if (${CORE} STREQUAL "LEAF")
    set(CORE_DIR ${ROOT_DIR}/src)
elseif (${CORE} STREQUAL "PIKA")
    set(CORE_DIR ${CMAKE_SOURCE_DIR}/pikapython/pikascript-core)
endif()
set(PIKA_LIB_DIR ${CMAKE_SOURCE_DIR}/pikapython/pikascript-lib)
set(PIKA_API_DIR ${CMAKE_SOURCE_DIR}/pikapython/pikascript-api)
set(PIKA_DIR ${CORE_DIR} ${PIKA_LIB_DIR} ${PIKA_API_DIR})
if (${MODE} STREQUAL "REPL")
    set(SRC_DIR ${CMAKE_SOURCE_DIR}/repl/src)
elseif (${MODE} STREQUAL "FILE")
    set(SRC_DIR ${CMAKE_SOURCE_DIR}/file/src)
endif()

# ============================= compile & link ============================= #
include_directories(${PIKA_DIR})

file(GLOB_RECURSE SRCS ${SRC_DIR}/*.h ${SRC_DIR}/*.c)
file(GLOB_RECURSE PIKA_LIB_SRCS ${PIKA_LIB_DIR}/*.h ${PIKA_LIB_DIR}/*.c)
file(GLOB_RECURSE PIKA_API_SRCS ${PIKA_API_DIR}/*.h ${PIKA_API_DIR}/*.c)

add_executable(${EXE_NAME} ${SRCS} ${PIKA_LIB_SRCS} ${PIKA_API_SRCS})

target_link_libraries(${EXE_NAME} ${CMAKE_PROJECT_NAME}-core pthread)

add_subdirectory(${CORE_DIR} core)