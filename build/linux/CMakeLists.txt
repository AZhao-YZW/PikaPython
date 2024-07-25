cmake_minimum_required(VERSION 3.10)
set(C_STANDARD 99)

project(LeafPython_linux)

include_directories(../../src)
aux_source_directory(. SRCS)
aux_source_directory(../../src SRCS)

add_executable(${PROJECT_NAME} ${SRCS}) # 生成可执行文件，这里程序名即为功能名

target_link_libraries(${PROJECT_NAME}) # 链接库
