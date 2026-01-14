Find all of the commnents in c++ files (cpp, h, and all the others), write them (no need to remove from c++ files or extracting), just reading and writing into one text file. Then in that file find the parts where we said like "for now", "implement later", "did for now", "will be implemented later", "todo", "fixme" and such meaning parts. And find them from the c++ files and do fix.
---

## Todo List for Dotlin Language Implementation

- [ ] is for not completed.
- [-] is for partially completed
- [x] is for completed.

### Critical Issues (NEW) ✅ FIXED

- ~~Fix class method execution - methods don't actually run~~ ✅ FIXED - Methods now execute their bodies correctly
- ~~Fix constructor field initialization - fields not set from constructor params~~ ✅ FIXED - Constructor parameters now properly initialize class fields
- ~~Fix 'this' keyword binding - 'this' doesn't work in methods~~ ✅ FIXED - The 'this' keyword now works correctly in methods
- [x] Split interpreter.cpp into modular files for maintainability ✅ COMPLETED

1. Method Chaining and Property Access
    - [x] Implement .toString() method for basic types (int, double, bool) ✅ WORKING
    - [x] Fix method chaining to properly chain return values ✅ WORKING
    - [-] Implement additional string methods (.substring(), .indexOf(), etc.) ⚠️ PARTIAL - Only .length and .toString() work
2. Command-Line Argument Support
    - [x] Connect command-line arguments from main.cpp to interpreter execution context ✅ WORKING
    - [x] Make command-line arguments available to fun main(args: Array<String>) function ✅ WORKING
    - [x] Implement Array type and Array operations ✅ WORKING
    - [x] Add args.contentToString() functionality ✅ WORKING
3. Advanced Formatting Functions
    - [-] Implement printf-style formatting functions ⚠️ MISSING - format() function not implemented
    - [-] Implement number formatting functions ⚠️ MISSING - toInt(), toDouble() not implemented
    - [-] Implement string formatting utilities ⚠️ PARTIAL - Basic concatenation works
4. Array Operations Support
    - [x] Implement Array data type ✅ WORKING
    - [x] Add array indexing operations ([]) ✅ WORKING
    - [x] Implement array methods (.size, .get(), .set(), etc.) ✅ WORKING
    - [x] Add array construction syntax ✅ WORKING
5. Complex Expression Handling
    - [x] Add proper null pointer checks in AST traversal ✅ WORKING
    - [x] Fix unique_ptr dereference issues in complex expressions ✅ WORKING
    - [x] Improve error handling for malformed expressions ✅ WORKING
    - [x] Add bounds checking for property access ✅ WORKING
6. Additional String Methods
    - [-] Implement .substring(start, end) ❌ MISSING
    - [-] Implement .indexOf(char/string) ❌ MISSING
    - [-] Implement .startsWith(), .endsWith() ❌ MISSING
    - [-] Implement .toUpperCase(), .toLowerCase() ❌ MISSING
    - [-] Implement .trim(), .split() ❌ MISSING
    - [x] Implement .length property ✅ WORKING
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