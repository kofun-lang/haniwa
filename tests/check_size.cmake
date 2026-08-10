if(NOT DEFINED BINARY)
  message(FATAL_ERROR "BINARY is required")
endif()
if(NOT DEFINED MAX_BYTES)
  set(MAX_BYTES 2097152)
endif()

file(SIZE "${BINARY}" actual_bytes)
if(actual_bytes GREATER MAX_BYTES)
  message(FATAL_ERROR
    "${BINARY} is ${actual_bytes} bytes; Stage 0 ceiling is ${MAX_BYTES}")
endif()
message(STATUS "Stage 0 binary size: ${actual_bytes}/${MAX_BYTES} bytes")
