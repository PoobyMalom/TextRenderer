cmake_minimum_required(VERSION 3.15)
project(FontRenderer CXX C)

# ==============================
# Compiler and flags
# ==============================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -g -D_THREAD_SAFE)
# C flags for GLAD (C file)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")

# ==============================
# Output layout: keep repo root clean
# ==============================
# When you configure in "build/", ${CMAKE_BINARY_DIR} == <repo>/build
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ==============================
# Include paths
# ==============================
include_directories(
    include
    /opt/homebrew/include/SDL2
    third_party/glad/include
)

# Library search path (macOS Homebrew SDL2)
link_directories(/opt/homebrew/lib)

# ==============================
# Sources shared by all targets (everything except main/test mains)
# ==============================
set(SHARED_SRC
    src/MovablePoint.cpp
    src/Helpers.cpp
    src/TTFHeader.cpp
    src/TTFTable.cpp
    src/HeadTable.cpp
    src/MaxpTable.cpp
    src/LocaTable.cpp
    src/CmapTable.cpp
    src/GlyphTable.cpp
    src/TTFFile.cpp
    src/SDLInitializer.cpp
    src/HheaTable.cpp
    src/NameTable.cpp
    src/PostTable.cpp
    src/HmtxTable.cpp
    src/KernTable.cpp
    src/GposTable.cpp
    src/GeometryUtils.cpp
    src/VertexList.cpp
    src/GlyphRenderer.cpp
)

# ==============================
# SDL2 (via pkg-config)
# ==============================
find_package(PkgConfig REQUIRED)
pkg_search_module(SDL2 REQUIRED sdl2)
include_directories(${SDL2_INCLUDE_DIRS})
link_directories(${SDL2_LIBRARY_DIRS})

# ==============================
# main
# ==============================
add_executable(main
    main.cpp
    ${SHARED_SRC}
)
target_link_libraries(main ${SDL2_LIBRARIES})

# ==============================
# test
# ==============================
add_executable(test
    test.cpp
    ${SHARED_SRC}
)
target_link_libraries(test ${SDL2_LIBRARIES})

# ==============================
# shader-test (SDL2 + GLAD + OpenGL)
# ==============================
find_package(OpenGL REQUIRED)
set(GLAD_SRC third_party/glad/src/glad.c)

add_executable(shader-test
    shader_test.cpp
    ${SHARED_SRC}
    ${GLAD_SRC}
)
target_include_directories(shader-test PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIRS}
    third_party/glad/include
    include
)
target_link_libraries(shader-test
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    dl
)

# ==============================
# Optional preprocessed output (PP=ON)
# Emits .i files into build/preprocessed/
# ==============================
option(PP "Emit preprocessed .i files for debugging" OFF)
if(PP)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/preprocessed)
    add_custom_target(preprocess
        COMMAND ${CMAKE_CXX_COMPILER} $<TARGET_PROPERTY:main,COMPILE_OPTIONS> -E ${CMAKE_SOURCE_DIR}/main.cpp
                -I${CMAKE_SOURCE_DIR}/include -o ${CMAKE_BINARY_DIR}/preprocessed/main.i
        COMMAND ${CMAKE_CXX_COMPILER} $<TARGET_PROPERTY:test,COMPILE_OPTIONS> -E ${CMAKE_SOURCE_DIR}/test.cpp
                -I${CMAKE_SOURCE_DIR}/include -o ${CMAKE_BINARY_DIR}/preprocessed/test.i
        COMMENT "Generating preprocessed source files (.i) in build/preprocessed/"
        VERBATIM
    )
endif()
