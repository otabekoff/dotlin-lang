# Dotlin Main Function

This document describes the main function entry point in Dotlin.

## Entry Point

Dotlin programs start execution from the `main` function, similar to Kotlin and Java.

### Basic Main Function
```kotlin
fun main() {
    println("Hello, World!")
}
```

**Parameters:**
- None

**Behavior:**
- Entry point for program execution
- Executes the statements in the function body
- Returns when all statements complete

### Main Function with Arguments
```kotlin
fun main(args: Array<String>) {
    println("Program arguments: " + args.contentToString())
}
```

**Parameters:**
- `args`: Array of command-line arguments passed to the program

**Behavior:**
- Entry point for program execution with command-line arguments
- Arguments are available in the `args` parameter
- Returns when all statements complete

## Usage Examples

### Simple Hello World
```kotlin
fun main() {
    println("Hello from Dotlin!")
}
```

### Using Command-Line Arguments
```kotlin
fun main(args: Array<String>) {
    if (args.size > 0) {
        println("First argument: " + args[0])
    } else {
        println("No arguments provided")
    }
}
```

### Complex Example with I/O
```kotlin
fun main() {
    println("Welcome to Dotlin!")
    print("Enter your name: ")
    val name = readln()
    println("Hello, " + name + "!")
}
```

## Implementation Status

- ✅ `fun main()` - Implemented
- ✅ `fun main(args: Array<String>)` - Syntax supported, arguments not yet accessible
- ⚠️ Command-line argument passing - Partially implemented
- ✅ Main function execution - Implemented