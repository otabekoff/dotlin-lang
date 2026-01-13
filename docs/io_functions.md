# Dotlin I/O Functions

This document describes the input/output functions available in Dotlin.

## Print Functions

### `println(value)`
Prints the specified value to standard output followed by a newline character.

**Syntax:**
```kotlin
println("Hello, World!")
println(42)
println(true)
```

**Parameters:**
- `value`: Any value (string, number, boolean) to print

**Behavior:**
- Converts the value to string representation
- Prints the string to standard output
- Adds a newline character at the end

### `print(value)`
Prints the specified value to standard output without adding a newline character.

**Syntax:**
```kotlin
print("Hello, ")
print("World!")
// Output: Hello, World!
```

**Parameters:**
- `value`: Any value (string, number, boolean) to print

**Behavior:**
- Converts the value to string representation
- Prints the string to standard output
- Does not add a newline character

## Input Functions

### `readln()`
Reads a line of text from standard input.

**Syntax:**
```kotlin
val input = readln()
```

**Return Value:**
- Returns the string read from standard input (without the newline character)

**Behavior:**
- Waits for user input
- Reads the entire line until newline character
- Returns the input as a string

## Usage Examples

### Basic Printing
```kotlin
fun main() {
    println("Hello, Dotlin!")
    print("Enter your name: ")
    val name = readln()
    println("Hello, " + name + "!")
}
```

### Combined I/O Operations
```kotlin
fun main() {
    println("Calculator Example")
    print("Enter first number: ")
    val num1 = readln()
    
    print("Enter second number: ")
    val num2 = readln()
    
    // Note: This would require string to number conversion
    println("You entered: " + num1 + " and " + num2)
}
```

## Implementation Status

- ✅ `println(value)` - Implemented
- ✅ `print(value)` - Implemented  
- ✅ `readln()` - Implemented
- ✅ String property access (`.length`) - Implemented
- ⚠️ Advanced formatting functions (`.toString()`) - Partially implemented
- ❌ Other advanced formatting functions - Planned