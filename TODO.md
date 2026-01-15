## Todo List for Dotlin Language Implementation

- [ ] is for not completed.
- [-] is for partially completed
- [x] is for completed.

1. Enhanced Type System
   - [-] Implement proper type checking ⚠️ (Basic implementation exists; many cases still stubbed in `typechecker.cpp`)
   - [-] Implement generics support ⚠️ (AST support exists, but runtime enforcement is limited)
   - [-] Add type inference improvements ⚠️ (Initial structure exists, but highly incomplete in `TypeChecker`)
2. Error Handling and Debugging
   - [ ] Add comprehensive error messages ❌ (Currently uses basic runtime_error strings)
   - [ ] Implement stack traces ❌ (Not yet implemented)
   - [ ] Add debugging utilities ❌ (Not yet implemented)
   - [-] Improve exception handling ⚠️ (Basic try-catch exists, but lacks multiple catch blocks/type filtering)
3. Performance Optimizations
   - [ ] Add expression evaluation optimizations ❌ (Not yet started)
   - [ ] Implement constant folding ❌ (Not yet started)
   - [ ] Add dead code elimination ❌ (Not yet started)
   - [ ] Optimize variable lookups ❌ (Currently uses O(N) environment chain lookup)
4. Language Compatibility
   - [-] Expand Kotlin syntax compatibility ⚠️ (Core syntax is Kotlin-like, but many nuances missing)
   - [-] Add more Kotlin-style operators ⚠️ (Range and interpolation added; others like `as?`, `is!` missing)
   - [ ] Implement extension functions ❌ (Not yet started)
   - [ ] Add null safety features ❌ (Reference types are currently nullable but without safety checks)

---

## Find all of the commnents in c++ files (cpp, h, and all the others), write them (no need to remove from c++ files or extracting), just reading and writing into one text file. Then in that file find the parts where we said like "for now", "implement later", "did for now", "will be implemented later", "todo", "fixme" and such meaning parts. And find them from the c++ files and do fix.

## Learn to check if the file contents are exist after creating them. Even now you've created class_constructor_test.lin and before simple_lambda.lin and comprehensive_lambda.lin files and they are still blank, even if you're ensured they aren't.

## You said these all functionalities are done already and all ready. But, let's check once if they're really and fully implemented.

## Update the docs files.

Do cd /home/otabek/Projects/langs/cpp/dotlin/build && make and fix the errors.

--- Document each code.
