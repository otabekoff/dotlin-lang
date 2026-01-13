option(DOTLIN_ENABLE_ASAN "Enable AddressSanitizer (non-MSVC)" OFF)
option(DOTLIN_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer (non-MSVC)" OFF)
option(DOTLIN_ENABLE_TSAN "Enable ThreadSanitizer (non-MSVC)" OFF)
option(DOTLIN_ENABLE_LSAN "Enable LeakSanitizer (non-MSVC)" OFF)

function(dotlin_apply_sanitizers target)
  if(MSVC)
    message(WARNING "Sanitizers are not supported on MSVC, ignoring sanitizer options")
    return()
  endif()

  set(sanitizers "")

  if(DOTLIN_ENABLE_ASAN)
    list(APPEND sanitizers "address")
  endif()

  if(DOTLIN_ENABLE_UBSAN)
    list(APPEND sanitizers "undefined")
  endif()

  if(DOTLIN_ENABLE_TSAN)
    list(APPEND sanitizers "thread")
  endif()

  if(DOTLIN_ENABLE_LSAN)
    list(APPEND sanitizers "leak")
  endif()

  if(sanitizers)
    list(JOIN sanitizers "," SANITIZER_STRING)
    target_compile_options(${target} PRIVATE -fsanitize=${SANITIZER_STRING})
    target_link_options(${target} PRIVATE -fsanitize=${SANITIZER_STRING})
  endif()
endfunction()
