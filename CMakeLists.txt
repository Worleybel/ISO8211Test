cmake_minimum_required(VERSION 3.15)
project(iso8211_test CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ISO8211 核心静态库，仅依赖 iso8211_port.h 这个最小适配层，不引入 GDAL CPL/VSI。
add_library(iso8211 STATIC
    ddfmodule.cpp
    ddfutils.cpp
    ddffielddefn.cpp
    ddfrecord.cpp
    ddffield.cpp
    ddfsubfielddefn.cpp
    ddfrecordindex.cpp
)

target_include_directories(iso8211 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Smoke test 可执行文件：创建一个 .ddf，写入记录后重新打开读取并校验。
add_executable(iso8211_test main.cpp)
target_link_libraries(iso8211_test PRIVATE iso8211)
