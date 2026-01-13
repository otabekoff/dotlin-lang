# Dotlin Language Features

This document describes the features currently implemented in the Dotlin programming language.

## Core Features

### Variables
- `val` for immutable variables
- `var` for mutable variables (planned)
- Type inference for integers, doubles, booleans, and strings

### Data Types
- Integer numbers (int)
- Floating-point numbers (double)
- Boolean values (true/false)
- Strings

### Operators
- Arithmetic: `+`, `-`, `*`, `/`, `%`, `++`, `--`, `+=`, `-=`, `*=`, `/=`, `%=`
- Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Logical: `&&`, `||`, `!`
- Assignment: `=`

### Control Flow
- If/else conditional statements

### Functions
- Basic function declarations (syntax support implemented)
- Function calls
- Built-in I/O functions: `println`, `print`, `readln`

### Comments
- Single-line comments using `//`

## Planned Features

### Main Function
- `fun main()` entry point support
- `fun main(args: Array<String>)` with command-line arguments (planned)

### Advanced Features
- Classes and objects
- Collections (arrays, lists, maps)
- Exception handling
- Generics