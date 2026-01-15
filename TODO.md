## Todo List for Dotlin Language Implementation

- [ ] is for not completed.
- [-] is for partially completed
- [x] is for completed.


1. Method Chaining and Property Access
    - [x] Implement .toString() method for basic types (int, double, bool) ✅ WORKING
    - [x] Fix method chaining to properly chain return values ✅ WORKING
    - [-] Implement additional string methods (.substring(), .indexOf(), etc.) ⚠️ PARTIAL - Only .length and .toString() work
2. Advanced Formatting Functions
    - [-] Implement printf-style formatting functions ⚠️ MISSING - format() function not implemented
    - [-] Implement number formatting functions ⚠️ MISSING - toInt(), toDouble() not implemented
    - [-] Implement string formatting utilities ⚠️ PARTIAL - Basic concatenation works
3. Additional String Methods
    - [-] Implement .substring(start, end) ❌ MISSING
    - [-] Implement .indexOf(char/string) ❌ MISSING
    - [-] Implement .startsWith(), .endsWith() ❌ MISSING
    - [-] Implement .toUpperCase(), .toLowerCase() ❌ MISSING
    - [-] Implement .trim(), .split() ❌ MISSING
    - [x] Implement .length property ✅ WORKING
4. Enhanced Type System
    - [x] Implement proper type checking
    - [x] Add explicit type annotations support
    - [x] Implement generics support
    - [x] Add type inference improvements
5. Control Flow Enhancements
    - [x] Implement while loops
    - [x] Implement for loops
    - [x] Implement when expressions (Kotlin equivalent of switch)
    - [x] Add exception handling support
6. Function System Improvements
    - [x] Implement user-defined functions beyond main
    - [x] Add function return type support
    - [x] Implement function overloading
    - [x] Add lambda expressions support
    - [x] Implement closure support for lambda expressions
7. Class and Object Support
    - [x] Implement class declarations
    - [x] Add constructor support
    - [ ] Implement inheritance
    - [x] Add property and method support in classes
8. Standard Library Functions
    - [ ] Implement math functions (sin, cos, sqrt, etc.)
    - [ ] Add file I/O operations
    - [ ] Implement date/time functions
    - [ ] Add collection utilities
9. Error Handling and Debugging
    - [ ] Add comprehensive error messages
    - [ ] Implement stack traces
    - [ ] Add debugging utilities
    - [ ] Improve exception handling
10. Performance Optimizations
    - [ ] Add expression evaluation optimizations
    - [ ] Implement constant folding
    - [ ] Add dead code elimination
    - [ ] Optimize variable lookups
11. Language Compatibility
    - [ ] Expand Kotlin syntax compatibility
    - [ ] Add more Kotlin-style operators
    - [ ] Implement extension functions
    - [ ] Add null safety features

---



Find all of the commnents in c++ files (cpp, h, and all the others), write them (no need to remove from c++ files or extracting), just reading and writing into one text file. Then in that file find the parts where we said like "for now", "implement later", "did for now", "will be implemented later", "todo", "fixme" and such meaning parts. And find them from the c++ files and do fix.
----
Learn to check if the file contents are exist after creating them. Even now you've created class_constructor_test.lin and before simple_lambda.lin and comprehensive_lambda.lin files and they are still blank, even if you're ensured they aren't.
----
You said these all functionalities are done already and all ready. But, let's check once if they're really and fully implemented.
----
Update the docs files.
---
Do cd /home/otabek/Projects/langs/cpp/dotlin/build && make and fix the errors.