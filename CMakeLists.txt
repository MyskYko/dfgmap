cmake_minimum_required(VERSION 3.8)
project(dfgmap CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-g -O3)

add_subdirectory(glucose)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB FILENAMES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(dfgmap ${FILENAMES})
target_link_libraries(dfgmap glucose)
target_include_directories(dfgmap PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/glucose)
