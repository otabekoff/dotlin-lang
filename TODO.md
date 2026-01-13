As, we're making our programming language the source fiels are being bigger, taller, longer. You can't process long files. Also, it is bad practice to have so long files. So, in src folder we can save main files but move their separatable parts into files in respective folders like taking some extractable parts of interpreter and moving to interpreter folder file and doing so for the whole projects files if applicable. We need to do these by thinking.
----
Find all of the commnents in c++ files (cpp, h, and all the others), write them (no need to remove from c++ files or extracting), just reading and writing into one text file. Then in that file find the parts where we said like "for now", "implement later", "did for now", "will be implemented later", "todo", "fixme" and such meaning parts. And find them from the c++ files and do fix.
---

## Todo List for Dotlin Language Implementation

- [ ] is not completed.
- [-] is partially completed
- [x] is completed.

1. Method Chaining and Property Access
    - [x] Implement .toString() method for basic types (int, double, bool)
    - [x] Fix method chaining to properly chain return values
    - [ ] Implement additional string methods (.substring(), .indexOf(), etc.)
2. Command-Line Argument Support
    - [x] Connect command-line arguments from main.cpp to interpreter execution context
    - [ ] Make command-line arguments available to fun main(args: Array<String>) function
    - [ ] Implement Array type and Array operations
    - [ ] Add args.contentToString() functionality
3. Advanced Formatting Functions
    - [ ] Implement printf-style formatting functions
    - [ ] Implement number formatting functions
    - [ ] Implement string formatting utilities
4. Array Operations Support
    - [ ] Implement Array data type
    - [ ] Add array indexing operations ([])
    - [ ] Implement array methods (.size, .get(), .set(), etc.)
    - [ ] Add array construction syntax
5. Complex Expression Handling
    - [ ] Add proper null pointer checks in AST traversal
    - [x] Fix unique_ptr dereference issues in complex expressions
    - [ ] Improve error handling for malformed expressions
    - [ ] Add bounds checking for property access
6. Additional String Methods
    - [ ] Implement .substring(start, end)
    - [ ] Implement .indexOf(char/string)
    - [ ] Implement .startsWith(), .endsWith()
    - [ ] Implement .toUpperCase(), .toLowerCase()
    - [ ] Implement .trim(), .split()
7. Enhanced Type System
    - [ ] Implement proper type checking
    - [ ] Add explicit type annotations support
    - [ ] Implement generics support
    - [ ] Add type inference improvements
8. Control Flow Enhancements
    - [ ] Implement while loops
    - [ ] Implement for loops
    - [ ] Implement when expressions (Kotlin equivalent of switch)
    - [ ] Add exception handling support
9. Function System Improvements
    - [ ] Implement user-defined functions beyond main
    - [ ] Add function return type support
    - [ ] Implement function overloading
    - [ ] Add lambda expressions support
10. Class and Object Support
    - [ ] Implement class declarations
    - [ ] Add constructor support
    - [ ] Implement inheritance
    - [ ] Add property and method support in classes
11. Standard Library Functions
    - [ ] Implement math functions (sin, cos, sqrt, etc.)
    - [ ] Add file I/O operations
    - [ ] Implement date/time functions
    - [ ] Add collection utilities
12. Error Handling and Debugging
    - [ ] Add comprehensive error messages
    - [ ] Implement stack traces
    - [ ] Add debugging utilities
    - [ ] Improve exception handling
13. Performance Optimizations
    - [ ] Add expression evaluation optimizations
    - [ ] Implement constant folding
    - [ ] Add dead code elimination
    - [ ] Optimize variable lookups
14. Language Compatibility
    - [ ] Expand Kotlin syntax compatibility
    - [ ] Add more Kotlin-style operators
    - [ ] Implement extension functions
    - [ ] Add null safety features
----
Update the docs files.