cmake_minimum_required(VERSION 3.16)
project(kal-lang)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# compiler options pseudo target
set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

add_library(compiler_flags INTERFACE)
target_compile_options(compiler_flags INTERFACE 
"$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wextra;-Wshadow;-Wformat=2;-Wunused>>"
"$<${msvc_like_cxx}:$<BUILD_INTERFACE:-W3>>"
)
target_compile_features(compiler_flags INTERFACE cxx_std_11)

# LLVM lib wrapper
add_library(llvm_wrap INTERFACE)
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
target_compile_definitions(llvm_wrap INTERFACE
    ${LLVM_DEFINITIONS_LIST}
)
target_include_directories(llvm_wrap INTERFACE ${LLVM_INCLUDE_DIRS})
llvm_map_components_to_libnames(llvm_libs support core irreader)
target_link_libraries(llvm_wrap INTERFACE ${llvm_libs})

# klang target
file(GLOB_RECURSE SOURCE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE HEADER_LIST ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

add_executable(klang ${SOURCE_LIST} ${HEADER_LIST})
target_include_directories(klang PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(klang PUBLIC compiler_flags llvm_wrap)

install(TARGETS klang DESTINATION bin)