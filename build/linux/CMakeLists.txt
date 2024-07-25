# ========================= cmake basic definition ========================= #
cmake_minimum_required(VERSION 3.10)
set(C_STANDARD 99)
project(LeafPython_linux)

# ================================ variables =============================== #
set(EXE_NAME "leafpython")
set(SRC_DIR  ${CMAKE_SOURCE_DIR}/src)
set(ROOT_DIR ${CMAKE_SOURCE_DIR}/../..)
set(CORE_DIR ${ROOT_DIR}/src)

# ============================= compile & link ============================= #
aux_source_directory(${SRC_DIR} SRCS)
aux_source_directory(${CORE_DIR} SRCS)
add_executable(${EXE_NAME} ${SRCS})
