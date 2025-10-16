# Compiler / flags
CXX       := g++
CXXFLAGS  := -std=c++17 -Wall -g -D_THREAD_SAFE \
             -Iinclude -I/opt/homebrew/include/SDL2 \
             -MMD -MP
LDFLAGS   := -L/opt/homebrew/lib -lSDL2

# --- Toggleable preprocessing dump ---
# Set PP=1 to also emit preprocessed .i files next to the objects
PP ?= 0
ifeq ($(PP),1)
  PP_SUFFIX := .i
  # Helper expands $@ (object path) to a sibling .i with same basename
  PP_CMD = $(CXX) $(CXXFLAGS) -E $< -o $(basename $@)$(PP_SUFFIX)
  SHADER_PP_CMD = $(CXX) $(SHADER_CXXFLAGS) -E $< -o $(basename $@)$(PP_SUFFIX)
else
  PP_CMD :=
  SHADER_PP_CMD :=
endif

# Targets
TARGET      := main
TEST_TARGET := test

# Build dir for non-main objects/dep files
OBJDIR := build

# Sources
SRCS := main.cpp \
        src/MovablePoint.cpp src/Helpers.cpp src/TTFHeader.cpp src/TTFTable.cpp \
        src/HeadTable.cpp src/MaxpTable.cpp src/LocaTable.cpp src/CmapTable.cpp \
        src/GlyphTable.cpp src/TTFFile.cpp src/SDLInitializer.cpp src/HheaTable.cpp \
				src/NameTable.cpp src/PostTable.cpp src/HmtxTable.cpp src/KernTable.cpp \
				src/GposTable.cpp src/GeometryUtils.cpp src/VertexList.cpp src/GlyphRenderer.cpp

TEST_SRCS := test.cpp \
             src/MovablePoint.cpp src/Helpers.cpp src/TTFHeader.cpp src/TTFTable.cpp \
             src/HeadTable.cpp src/MaxpTable.cpp src/LocaTable.cpp src/CmapTable.cpp \
             src/GlyphTable.cpp src/TTFFile.cpp src/SDLInitializer.cpp src/HheaTable.cpp \
						 src/NameTable.cpp src/PostTable.cpp src/HmtxTable.cpp src/KernTable.cpp \
						 src/GposTable.cpp src/GeometryUtils.cpp src/VertexList.cpp src/GlyphRenderer.cpp

# Split out main.cpp so its .o/.d stay in project root
NONMAIN_SRCS := $(filter-out main.cpp,$(SRCS))
MAIN_OBJ     := main.o
MAIN_DEP     := $(MAIN_OBJ:.o=.d)

# Objects/Deps for non-main sources go under build/
OBJS := $(NONMAIN_SRCS:%.cpp=$(OBJDIR)/%.o)
DEPS := $(OBJS:.o=.d)

# For test target: test.o stays in root; rest reuse OBJS in build/
TEST_MAIN_OBJ := test.o
TEST_MAIN_DEP := $(TEST_MAIN_OBJ:.o=.d)
TEST_OBJS     := $(OBJS)        # reuse non-main objs
TEST_DEPS     := $(DEPS)

.PHONY: all clean
all: $(TARGET)

# Link final binaries in project root
$(TARGET): $(MAIN_OBJ) $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(TEST_TARGET): $(TEST_MAIN_OBJ) $(TEST_OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

# --- Compile rules ---

# main.cpp -> main.o (and main.d) in root
$(MAIN_OBJ): main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
	$(PP_CMD)

# test.cpp -> test.o (and test.d) in root
$(TEST_MAIN_OBJ): test.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
	$(PP_CMD)

# Any other .cpp -> build/…/.o (and build/…/.d)
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	$(PP_CMD)

# Housekeeping
clean:
	rm -rf $(OBJDIR) $(TARGET) $(TEST_TARGET) \
	       $(MAIN_OBJ) $(MAIN_DEP) \
	       $(TEST_MAIN_OBJ) $(TEST_MAIN_DEP)

# Include auto-generated dependency files (ok if missing)
-include $(DEPS) $(MAIN_DEP) $(TEST_DEPS) $(TEST_MAIN_DEP)

# ===== Shader test (SDL2 + GLAD) =====
SHADER_TARGET   := shader-test
GLAD_DIR        := third_party/glad
SHADER_OBJDIR   := build/shader

# Reuse all sources used by 'test', EXCEPT test.cpp (it has its own main)
SHADER_SHARED_SRCS := $(filter-out test.cpp,$(TEST_SRCS))
SHADER_CPP         := shader_test.cpp
SHADER_GLAD_C      := $(GLAD_DIR)/src/glad.c

# All C++ sources we want for shader-test
SHADER_ALL_CPP := $(SHADER_SHARED_SRCS) $(SHADER_CPP)

# Objects (mirror folder structure under build/shader/)
SHADER_OBJS := $(SHADER_ALL_CPP:%.cpp=$(SHADER_OBJDIR)/%.o) \
               $(SHADER_OBJDIR)/glad.o
SHADER_DEPS := $(SHADER_OBJS:.o=.d)

CC      := gcc
CFLAGS  := -Wall -Wextra -g -MMD -MP -I$(GLAD_DIR)/include

# Use pkg-config for SDL2 includes/libs. Also include your project headers.
SHADER_CXXFLAGS := -std=c++17 -Wall -Wextra -g -MMD -MP \
                   $(shell pkg-config --cflags sdl2) \
                   -I$(GLAD_DIR)/include -Iinclude
SHADER_LDFLAGS  := $(shell pkg-config --libs sdl2) -lGL -ldl

.PHONY: shader-test
shader-test: $(SHADER_TARGET)

$(SHADER_TARGET): $(SHADER_OBJS)
	$(CXX) -o $@ $^ $(SHADER_LDFLAGS)

# Generic rule to compile ANY project .cpp into build/shader/…
$(SHADER_OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(SHADER_CXXFLAGS) -c $< -o $@
	$(SHADER_PP_CMD)

# Compile GLAD (C file)
$(SHADER_OBJDIR)/glad.o: $(SHADER_GLAD_C)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(SHADER_DEPS)
