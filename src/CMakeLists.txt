add_library(dotlin_lib)
add_library(dotlin::lib ALIAS dotlin_lib)

# Define sources with explicit paths
set(SOURCES
  lexer.cpp
  parser.cpp
  interpreter/environment.cpp
  interpreter/utils.cpp
  interpreter/main.cpp
  interpreter/evaluator.cpp
  interpreter/executer.cpp
  interpreter/typechecker.cpp
  interpreter/builtins.cpp
  interpreter/interpreter.cpp
  interpreter/constant_folder.cpp
  interpreter/dead_code_elimination.cpp
)

# Set sources for the library
set_target_properties(dotlin_lib PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
)

target_sources(dotlin_lib
  PRIVATE
  ${SOURCES}
)

target_compile_features(dotlin_lib PUBLIC cxx_std_20)

target_include_directories(dotlin_lib
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(dotlin_lib
  PUBLIC
  dotlin_warnings
)

dotlin_apply_sanitizers(dotlin_lib)
