## Todo List for Dotlin Language Implementation

- [ ] is for not completed.
- [-] is for partially completed
- [x] is for completed.

- [x] Phase 15: Performance Optimizations
  - [x] Implement Constant Folding (evaluate constant expressions statically)
  - [x] Implement Dead Code Elimination
  - [x] Optimize Environment lookups (use hashes or pre-indexed scopes)

- [ ] Phase 16: Advanced Kotlin Features
  - [x] Extension Functions (`fun Type.name()`)
  - [ ] Null Safety (`String?`, `?.`, `!!`)
  - [ ] Full Generics Enforcement

- [ ] Phase 17: Language Compatibility & Audit
  - [x] Audit codebase for "todo", "for now", "fixme" comments and address them
  - [-] Expand Kotlin syntax compatibility (operators like `as?`, `is!`)
  - [ ] Expand test suite for 100% Kotlin compliance

---

## Find all of the commnents in c++ files (cpp, h, and all the others), write them (no need to remove from c++ files or extracting), just reading and writing into one text file. Then in that file find the parts where we said like "for now", "implement later", "did for now", "will be implemented later", "todo", "fixme" and such meaning parts. And find them from the c++ files and do fix.

## Learn to check if the file contents are exist after creating them. Even now you've created class_constructor_test.lin and before simple_lambda.lin and comprehensive_lambda.lin files and they are still blank, even if you're ensured they aren't.

## You said these all functionalities are done already and all ready. But, let's check once if they're really and fully implemented.

## Update the docs files.

Do cd /home/otabek/Projects/langs/cpp/dotlin/build && make and fix the errors.

--- Document each code.

---

otabek@mohirlab:~/Projects/langs/cpp/dotlin/build/apps$ ./dotlin .
./../tests/benchmark_scope.lin
Time taken: 35957ms
Execution result: 

otabek@mohirlab:~/Projects/langs/cpp/dotlin/tests$ python3 benchmark_scope.py
Time taken: 48 ms

g++ -O3 -o tests/benchmark_scope_cpp tests/benchmark_scope.cpp && ./tests/benchmark_scope_cpp

Time taken: 16ms

otabek@mohirlab:~/Projects/langs/cpp/dotlin/tests$ kotlinc benchma
rk_scope.kt -include-runtime -d benchmark.jar
otabek@mohirlab:~/Projects/langs/cpp/dotlin/tests$ java -jar benchmark.jar
Time taken: 3ms

otabek@mohirlab:~/Projects/langs/cpp/dotlin/tests$ kotlinc -script benchmark_scope.kts
Time taken: 5ms
otabek@mohirlab:~/Projects/langs/cpp/dotlin/tests$ 


Why so much difference? Why so diversity? Can we make it fast? Plan to make dotlin very fast? AOT it should be. And very fast and quick. Not only in this case, but for everything.


---

Clean the project from unnecessary files

---

Make production ready and deployable and distributable like other programming languages with best practices.