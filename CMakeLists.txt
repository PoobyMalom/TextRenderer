cmake_minimum_required(VERSION 3.15)
project(FontRenderer CXX C)

# =========================================
# Compiler and flags
# =========================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -g -D_THREAD_SAFE)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")

# Export compile_commands.json for clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =========================================
# Output layout: keep repo root clean
# =========================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# =========================================
# Include paths
# =========================================
include_directories(
    include
    /opt/homebrew/include/SDL2
    third_party/glad/include
)

# Library search path (macOS Homebrew SDL2)
link_directories(/opt/homebrew/lib)

# =========================================
# Sources shared by all targets (everything except main/test mains)
# =========================================
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

# =========================================
# SDL2 (via pkg-config)
# =========================================
find_package(PkgConfig REQUIRED)
pkg_search_module(SDL2 REQUIRED sdl2)
include_directories(${SDL2_INCLUDE_DIRS})
link_directories(${SDL2_LIBRARY_DIRS})

# =========================================
# main
# =========================================
add_executable(main
    main.cpp
    ${SHARED_SRC}
)
target_link_libraries(main ${SDL2_LIBRARIES})

# =========================================
# test
# =========================================
add_executable(test
    test.cpp
    ${SHARED_SRC}
)
target_link_libraries(test ${SDL2_LIBRARIES})

# =========================================
# shader-test (SDL2 + GLAD + OpenGL)
# =========================================
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

# =========================================
# Optional preprocessed output (PP=ON)
# Emits .i files into build/preprocessed/
# =========================================
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

# =========================================
# Clang tooling (format + tidy for *your* code only)
# =========================================
# Collect user files: src/, include/, and the three mains; exclude third_party/
# (GLOB is fine here since it's tooling-only, not for targets)
file(GLOB_RECURSE USER_HEADERS
    CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/include/*.h
    ${CMAKE_SOURCE_DIR}/include/*.hpp
    ${CMAKE_SOURCE_DIR}/src/*.h
    ${CMAKE_SOURCE_DIR}/src/*.hpp
)
set(USER_SOURCES
    ${SHARED_SRC}
    ${CMAKE_SOURCE_DIR}/main.cpp
    ${CMAKE_SOURCE_DIR}/test.cpp
    ${CMAKE_SOURCE_DIR}/shader_test.cpp
)
set(ALL_USER_FILES ${USER_SOURCES} ${USER_HEADERS})
# Filter out anything under third_party just in case
list(FILTER ALL_USER_FILES EXCLUDE REGEX ".*/third_party/.*")

# --- clang-format ---
find_program(CLANG_FORMAT_EXE NAMES clang-format)
if(CLANG_FORMAT_EXE)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i --style=file ${ALL_USER_FILES}
        COMMENT "Running clang-format on project sources (excluding third_party)"
        VERBATIM
    )
endif()

# --- clang-tidy ---
option(ENABLE_CLANG_TIDY "Run clang-tidy during build" OFF)

# Prefer run-clang-tidy if present (better parallelism), otherwise fall back.
find_program(RUN_CLANG_TIDY_EXE NAMES run-clang-tidy run-clang-tidy.py)
find_program(CLANG_TIDY_EXE NAMES clang-tidy)

if(RUN_CLANG_TIDY_EXE AND CLANG_TIDY_EXE)
    add_custom_target(tidy
        COMMAND ${RUN_CLANG_TIDY_EXE}
                -p ${CMAKE_BINARY_DIR}
                -header-filter='^(src|include)/'
                -quiet
                ${ALL_USER_FILES}
        COMMENT "Running run-clang-tidy on project sources (excluding third_party)"
        VERBATIM
    )
elseif(CLANG_TIDY_EXE)
    # Single command; may be long, but works.
    add_custom_target(tidy
        COMMAND ${CLANG_TIDY_EXE}
                -p ${CMAKE_BINARY_DIR}
                -header-filter=^(src|include)/
                --quiet
                ${ALL_USER_FILES}
        COMMENT "Running clang-tidy on project sources (excluding third_party)"
        VERBATIM
    )
endif()

# Optionally attach clang-tidy to targets during build (opt-in)
if(ENABLE_CLANG_TIDY AND CLANG_TIDY_EXE)
    set(_TIDY_ARGS "-p;${CMAKE_BINARY_DIR};-header-filter=^(src|include)/;--quiet")
    set_target_properties(main PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_EXE};${_TIDY_ARGS}")
    set_target_properties(test PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_EXE};${_TIDY_ARGS}")
    set_target_properties(shader-test PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_EXE};${_TIDY_ARGS}")
endif()

# Keep compile_commands.json easy to find at repo root
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_custom_target(link-compile-commands ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink
          ${CMAKE_BINARY_DIR}/compile_commands.json
          ${CMAKE_SOURCE_DIR}/compile_commands.json
  COMMENT "Symlinking compile_commands.json to project root"
  VERBATIM)
