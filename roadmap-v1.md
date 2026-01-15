# Dotlin v1.0 Roadmap

## Phase 1: Performance & Architecture (Highest Priority)

Currently, Dotlin is ~10,000x slower than native C++ due to its Tree-Walking nature.

- [ ] **Bytecode VM**: Replace recursive evaluation with a register-based virtual machine.
- [ ] **Pre-indexed Lookups**: Eliminate string-based environment hashes at runtime.
- [ ] **AOT Compilation**: Implementation of an LLVM-based backend for native execution.

## Phase 2: Full OOP Support

Extend the class system to support full Kotlin-like object orientation.

- [ ] **Interfaces**: Support for `interface` and multiple implementation.
- [ ] **Abstract Classes**: Support for `abstract class` and members.
- [ ] **Visibility Modifiers**: Implementation of `private`, `protected`, `public`, `internal`.
- [ ] **Properties**: Support for custom getters and setters.

## Phase 3: Language Safety & Correctness

- [ ] **Null Safety**: Implementation of nullable types (`T?`), safe calls (`?.`), and elvis operator (`?:`).
- [ ] **Generics Enforcement**: Runtime/Compile-time check for generic type constraints.
- [ ] **Exception Enhancements**: Improved stack traces and error messages with context snippets.

## Phase 4: Standard Library & Tooling

- [ ] **Scope Functions**: `apply`, `run`, `let`, `also`, `with`.
- [ ] **Collections**: Fully featured `List`, `Map`, `Set` wrappers.
- [ ] **REPL**: Interactive shell for quick testing.
- [ ] **Package Manager**: Basic dependency management for `.lin` libraries.
