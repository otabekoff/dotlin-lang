# Dotlin - Native Kotlin Syntax Implementation in C++

Dotlin is a programming language that provides native 1:1 syntax compatibility with Kotlin without depending on the JVM. The language is implemented as a standalone interpreter and runtime environment using C++.

## Language Design

### Compilation Model
Dotlin is an **interpreted language** (with potential for JIT compilation in the future) that executes Kotlin-like syntax directly. The implementation approach:
- Parses source code to an Abstract Syntax Tree (AST)
- Interprets the AST directly without intermediate code generation
- Provides immediate execution feedback
- Enables rapid development and testing cycles
- Offers a runtime environment that mimics Kotlin's behavior

### Language Features

Dotlin supports the following Kotlin-like features:

**Core Features:**
- Type inference (`val` and `var` declarations)
- Null safety with nullable types
- Extension functions
- Data classes (planned)
- Coroutines (planned)
- Higher-order functions and lambdas (planned)
- Smart casts
- String templates (planned)
- When expressions (pattern matching)
- Ranges and progression

**Object-Oriented Features:**
- Classes and inheritance
- Interfaces
- Abstract classes
- Enum classes
- Sealed classes
- Nested and inner classes
- Properties with getters/setters
- Constructors (primary and secondary)

**Functional Features:**
- First-class functions
- Function types
- Inline functions
- Tail-recursive functions
- Operator overloading
- Higher-order functions
- Lambda expressions
- Extension functions

### Type System

Dotlin implements a static type system similar to Kotlin with:
- Primitive types (Int, Double, Boolean, String, etc.)
- Nullable and non-nullable types
- Generic type support
- Type inference capabilities
- Collection types (List, Map, Set - planned)

### Runtime Environment

The Dotlin interpreter provides:
- Variable scoping with lexical environments
- Expression evaluation engine
- Control flow execution
- Error handling and reporting
- Memory management for runtime values

## Current Implementation Status

Dotlin currently implements:
- Complete lexer with support for all Kotlin keywords and operators
- Parser with AST generation for basic constructs
- Interpreter with visitor pattern for AST traversal
- Runtime value representation using std::variant
- Variable declaration and access
- Basic arithmetic and logical expressions
- Function declarations (basic)
- If/else control flow
- Block scoping

Planned features:
- Full function implementation with parameters and return values
- Class and object system
- More comprehensive error handling
- Standard library implementation
- Advanced collection operations
- Coroutines support
- More complete type system

## Build Instructions

### Prerequisites
- CMake 3.24 or higher
- C++20 compatible compiler (GCC, Clang, or MSVC)

### Building

```bash
mkdir -p build
export PATH=$HOME/.local/bin:$PATH   # if you installed CMake locally
cmake -S . -B build
cmake --build build
```

### Build Options

The project supports several CMake options that can be configured during the cmake configuration step:

- `-DDOTLIN_BUILD_TESTS=ON/OFF`: Enable/disable building tests (default: ON)
- `-DDOTLIN_BUILD_APPS=ON/OFF`: Enable/disable building CLI apps (default: ON)
- `-DCMAKE_BUILD_TYPE=Debug/Release/RelWithDebInfo/MinSizeRel`: Set build type (default: Debug)
- `-DDOTLIN_ENABLE_ASAN=ON/OFF`: Enable AddressSanitizer (non-MSVC only, default: OFF)
- `-DDOTLIN_ENABLE_UBSAN=ON/OFF`: Enable UndefinedBehaviorSanitizer (non-MSVC only, default: OFF)
- `-DDOTLIN_ENABLE_TSAN=ON/OFF`: Enable ThreadSanitizer (non-MSVC only, default: OFF)
- `-DDOTLIN_ENABLE_LSAN=ON/OFF`: Enable LeakSanitizer (non-MSVC only, default: OFF)

Example with custom options:
```bash
cmake -S . -B build -DDOTLIN_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release -DDOTLIN_ENABLE_ASAN=ON
```

## Run

```bash
./build/dotlin
```

## Run Tests

If tests are built (enabled by default), you can run them after building:
```bash
cmake --build build
cd build
ctest
```

## Structure

- `include/dotlin` — public headers for lexer, parser, interpreter
- `src` — implementation sources
- `apps` — application executables
- `tests` — unit tests
- `cmake` — CMake modules for warnings and options
- `main.cpp` — small runner showing usage

## Development Notes

The project uses a modular CMake structure:
- Main library: `dotlin_lib` (available as alias `dotlin::lib`)
- Executable: `dotlin`
- Tests: `dotlin_simple_tests` (if enabled)

The code follows C++20 standards and uses modern CMake practices.

## Next steps

- Expand the `lex`, `parse`, and `interpret` functions.
- Add a tokenizer with positions, error reporting and unit tests.
- Integrate a testing framework (Catch2/GoogleTest) if desired.
