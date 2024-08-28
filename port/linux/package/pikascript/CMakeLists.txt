#设置 BINARY 为项目名IndexProject
set(BINARY ${CMAKE_PROJECT_NAME})
set(CMAKE_C_FLAGS "-Wno-error=deprecated-declarations -Wno-deprecated-declarations")

add_definitions(-D"bool=uint8_t" -D"true=1" -D"false=0")
set(CORE_DIR ${CMAKE_SOURCE_DIR}/package/pikascript/pikascript-core)
set(PARSER_DIR ${CORE_DIR}/parser)
set(OBJECT_DIR ${CORE_DIR}/object)
set(VM_DIR ${CORE_DIR}/vm)
set(PIKA_LIB_DIR ${CMAKE_SOURCE_DIR}/package/pikascript/pikascript-lib)
set(PIKA_API_DIR ${CMAKE_SOURCE_DIR}/package/pikascript/pikascript-api)
set(PIKA_DIR ${PARSER_DIR} ${OBJECT_DIR} ${VM_DIR} ${CORE_DIR}
             ${PIKA_LIB_DIR} ${PIKA_API_DIR})

include_directories(${PIKA_DIR})

file(GLOB_RECURSE PIKA_LIB_SRCS ${PIKA_LIB_DIR}/*.h ${PIKA_LIB_DIR}/*.c)
file(GLOB_RECURSE PIKA_API_SRCS ${PIKA_API_DIR}/*.h ${PIKA_API_DIR}/*.c)
file(GLOB_RECURSE SOURCES *.h *.c)

# support <math.h>
link_libraries(m lua5.3 pthread mbedcrypto ${BINARY}-lvgl)

add_library(${BINARY}-linux ${SOURCES} ${PIKA_LIB_SRCS} ${PIKA_API_SRCS})
target_link_libraries(${BINARY}-linux ${BINARY}-core pthread)

add_subdirectory(${CORE_DIR} core)