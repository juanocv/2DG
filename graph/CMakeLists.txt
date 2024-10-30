project(graph)
add_executable(${PROJECT_NAME} main.cpp window.cpp)
enable_abcg(${PROJECT_NAME})

# Detect if we are using Emscripten
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Emscripten")
  set(USING_EMSCRIPTEN TRUE)
else()
  set(USING_EMSCRIPTEN FALSE)
endif()

if(USING_EMSCRIPTEN)
  # Enable WebGL 2.0
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_WEBGL2=1")
  
  # Optional: Ensure correct GLES version is set
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s FULL_ES3=1")
  
  # Optional: Enable WASM (should be default)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s WASM=1")
  
  # Optional: Add any other necessary linker flags
endif()