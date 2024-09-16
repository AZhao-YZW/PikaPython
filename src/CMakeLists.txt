#设置 BINARY 为项目名IndexProject
set(BINARY ${CMAKE_PROJECT_NAME})

add_definitions(-D"bool=uint8_t" -D"true=1" -D"false=0")

set(PARSER_DIR ./parser)
set(OBJECT_DIR ./object)
set(VM_DIR ./vm)
set(LIB_DIR ./lib)
include_directories(${PARSER_DIR} ${OBJECT_DIR} ${VM_DIR} ${LIB_DIR})

file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true 
     *.h *.c 
     ${PARSER_DIR}/*.h ${PARSER_DIR}/*.c 
     ${OBJECT_DIR}/*.h ${OBJECT_DIR}/*.c
     ${VM_DIR}/*.h ${VM_DIR}/*.c
     ${LIB_DIR}/*.h ${LIB_DIR}/*.c)

add_library(${BINARY}-core 
            STATIC 
            ${SOURCES})
