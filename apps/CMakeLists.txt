# Create the main executable
add_executable(dotlin ../main.cpp)

# Link the library
target_link_libraries(dotlin PRIVATE dotlin::lib)

dotlin_apply_sanitizers(dotlin)

# Set C++ standard for executable
set_target_properties(dotlin PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
