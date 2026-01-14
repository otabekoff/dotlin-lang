Find all of the commnents in c++ files (cpp, h, and all the others), write them (no need to remove from c++ files or extracting), just reading and writing into one text file. Then in that file find the parts where we said like "for now", "implement later", "did for now", "will be implemented later", "todo", "fixme" and such meaning parts. And find them from the c++ files and do fix.
---

## Todo List for Dotlin Language Implementation

- [ ] is for not completed.
- [-] is for partially completed
- [x] is for completed.

## Critical Issues (NEW)
- [x] Fix class method execution - methods don't actually run
- [x] Fix constructor field initialization - fields not set from constructor params
- [x] Fix 'this' keyword binding - 'this' doesn't work in methods
- [ ] Split interpreter.cpp into modular files for maintainability

1. Method Chaining and Property Access
    - [x] Implement .toString() method for basic types (int, double, bool)
    - [x] Fix method chaining to properly chain return values
    - [x] Implement additional string methods (.substring(), .indexOf(), etc.)
2. Command-Line Argument Support
    - [x] Connect command-line arguments from main.cpp to interpreter execution context
    - [x] Make command-line arguments available to fun main(args: Array<String>) function
    - [-] Implement Array type and Array operations
    - [x] Add args.contentToString() functionality
3. Advanced Formatting Functions
    - [x] Implement printf-style formatting functions
    - [x] Implement number formatting functions
    - [x] Implement string formatting utilities
4. Array Operations Support
    - [-] Implement Array data type
    - [x] Add array indexing operations ([]) 
    - [x] Implement array methods (.size, .get(), .set(), etc.)
    - [x] Add array construction syntax
5. Complex Expression Handling
    - [x] Add proper null pointer checks in AST traversal
    - [x] Fix unique_ptr dereference issues in complex expressions
    - [x] Improve error handling for malformed expressions
    - [x] Add bounds checking for property access
6. Additional String Methods
    - [x] Implement .substring(start, end)
    - [x] Implement .indexOf(char/string)
    - [x] Implement .startsWith(), .endsWith()
    - [x] Implement .toUpperCase(), .toLowerCase()
    - [x] Implement .trim(), .split()
7. Enhanced Type System
    - [x] Implement proper type checking
    - [x] Add explicit type annotations support
    - [x] Implement generics support
    - [x] Add type inference improvements
8. Control Flow Enhancements
    - [x] Implement while loops
    - [x] Implement for loops
    - [x] Implement when expressions (Kotlin equivalent of switch)
    - [x] Add exception handling support
9. Function System Improvements
    - [x] Implement user-defined functions beyond main
    - [x] Add function return type support
    - [x] Implement function overloading
    - [x] Add lambda expressions support
    - [x] Implement closure support for lambda expressions
10. Class and Object Support
    - [x] Implement class declarations
    - [x] Add constructor support
    - [ ] Implement inheritance
    - [x] Add property and method support in classes
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
Learn to check if the file contents are exist after creating them. Even now you've created class_constructor_test.lin and before simple_lambda.lin and comprehensive_lambda.lin files and they are still blank, even if you're ensured they aren't.
----
You said these all functionalities are done already and all ready. But, let's check once if they're really and fully implemented.
----
Update the docs files.
---
Do cd /home/otabek/Projects/langs/cpp/dotlin/build && make and fix the errors.