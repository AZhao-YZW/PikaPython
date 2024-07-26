# ========================= cmake basic definition ========================= #
cmake_minimum_required(VERSION 3.10)
set(C_STANDARD 99)
project(LeafPython_linux)

# ================================ variables =============================== #
set(EXE_NAME "leafpython")

set(PIKA_GEN_DIR ${CMAKE_SOURCE_DIR}/pikascript-core
                 ${CMAKE_SOURCE_DIR}/pikascript-lib
                 ${CMAKE_SOURCE_DIR}/pikascript-api)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

# ============================= compile & link ============================= #
include_directories(${PIKA_GEN_DIR})

# pika generated files call each other, so use file(GLOB_RECURSE xxx)
# aux_source_directory(${CMAKE_SOURCE_DIR}/pikascript-core SRCS)
# aux_source_directory(${CMAKE_SOURCE_DIR}/pikascript-lib SRCS)
# aux_source_directory(${CMAKE_SOURCE_DIR}/pikascript-api SRCS)
# aux_source_directory(${SRC_DIR} SRCS)
file(GLOB_RECURSE SRCS *.h *.c)

add_executable(${EXE_NAME} ${SRCS})

target_link_libraries(${EXE_NAME} ${CMAKE_PROJECT_NAME}-core)

add_subdirectory(${CMAKE_SOURCE_DIR}/pikascript-core)