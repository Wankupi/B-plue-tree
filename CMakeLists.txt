cmake_minimum_required(VERSION 3.15)
project(B+tree)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
include_directories(src)

enable_testing()
set(files_prefix "${CMAKE_CURRENT_SOURCE_DIR}/test")
file(GLOB_RECURSE CPPs "${files_prefix}/**.cpp")

foreach (cpp_file ${CPPs})
    string(REGEX REPLACE "/[a-zA-Z]*\\.cpp" "" fpath ${cpp_file})
    string(REPLACE "${files_prefix}/" "" testname "${fpath}")
    add_executable(${testname} ${cpp_file})
endforeach ()
