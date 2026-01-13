cmake_minimum_required(VERSION 3.24)
project(
  dotlin
  VERSION 0.1.0
  DESCRIPTION "Dotlin Programming Language"
  HOMEPAGE_URL "https://github.com/your-repo/dotlin"
  LANGUAGES CXX
)

# ---- Project-wide Options ----
option(DOTLIN_BUILD_TESTS "Build tests" ON)
option(DOTLIN_BUILD_APPS "Build CLI apps" ON)

# ---- C++ Standard Configuration ----
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Export Compile Commands for IDE Support ----
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default build type for single-config generators (Ninja/Unix Makefiles)
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# ---- Module Path and Custom Modules ----
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DotlinWarnings)
include(DotlinOptions)

# ---- Subdirectories ----
add_subdirectory(src)

add_executable(test_dotlin test_dotlin.cpp)
target_link_libraries(test_dotlin PRIVATE dotlin_lib)

add_executable(simple_test simple_test.cpp)
target_link_libraries(simple_test PRIVATE dotlin_lib)

add_executable(lexer_test lexer_test.cpp)
target_link_libraries(lexer_test PRIVATE dotlin_lib)

if(DOTLIN_BUILD_APPS)
  add_subdirectory(apps)
endif()

if(DOTLIN_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
