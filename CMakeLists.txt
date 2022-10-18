cmake_minimum_required(VERSION 3.16)
project(kal-lang)

file(GLOB_RECURSE SOURCES_LIST ${CMAKE_SOURCE_DIR}/src/*.cpp)
set(LLVM_INCLUDE_PATH D:\\Programs\\LLVM\\include)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic)
    if(CMAKE_BUILD_TYPE MATCHES "Release")
        message("release mode")
        add_compile_options(-Werror)    
    endif()
    
endif()

add_executable(klang ${SOURCES_LIST})
target_include_directories(klang BEFORE PRIVATE
    ${LLVM_INCLUDE_PATH}
    ${CMAKE_SOURCE_DIR}/include   
)
