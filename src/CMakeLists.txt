#设置 BINARY 为项目名IndexProject
set(BINARY ${CMAKE_PROJECT_NAME})

add_definitions(-D"bool=uint8_t" -D"true=1" -D"false=0")

set(PARSER_DIR ./parser)
include_directories(${PARSER_DIR})

file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true *.h *.c ${PARSER_DIR}/*.h ${PARSER_DIR}/*.c)

add_library(${BINARY}-core 
            STATIC 
            ${SOURCES})
