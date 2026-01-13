add_library(dotlin_warnings INTERFACE)

if(MSVC)
  # Microsoft Visual C++ warnings
  target_compile_options(dotlin_warnings INTERFACE
    /W4
    /permissive-
  )
else()
  # GCC/Clang warnings
  target_compile_options(dotlin_warnings INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wdouble-promotion
  )
endif()

# Set warning level for all configurations
set_target_properties(dotlin_warnings PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
)
