# Compiler / flags
CXX       := g++
CXXFLAGS  := -std=c++17 -Wall -g -D_THREAD_SAFE \
             -Iinclude -I/opt/homebrew/include/SDL2 \
             -MMD -MP
LDFLAGS   := -L/opt/homebrew/lib -lSDL2

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
				src/NameTable.cpp src/PostTable.cpp src/HmtxTable.cpp src/KernTable.cpp

TEST_SRCS := test.cpp \
             src/MovablePoint.cpp src/Helpers.cpp src/TTFHeader.cpp src/TTFTable.cpp \
             src/HeadTable.cpp src/MaxpTable.cpp src/LocaTable.cpp src/CmapTable.cpp \
             src/GlyphTable.cpp src/TTFFile.cpp src/SDLInitializer.cpp src/HheaTable.cpp \
						 src/NameTable.cpp src/PostTable.cpp src/HmtxTable.cpp src/KernTable.cpp

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

# test.cpp -> test.o (and test.d) in root
$(TEST_MAIN_OBJ): test.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Any other .cpp -> build/…/.o (and build/…/.d)
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Housekeeping
clean:
	rm -rf $(OBJDIR) $(TARGET) $(TEST_TARGET) \
	       $(MAIN_OBJ) $(MAIN_DEP) \
	       $(TEST_MAIN_OBJ) $(TEST_MAIN_DEP)

# Include auto-generated dependency files (ok if missing)
-include $(DEPS) $(MAIN_DEP) $(TEST_DEPS) $(TEST_MAIN_DEP)

# ===== Shader test (SDL2 + GLAD) =====
SHADER_TARGET   := shader-test
SHADER_SRCS     := shader_test.cpp          # put the sample code in this file
SHADER_OBJDIR   := build/shader
SHADER_OBJS     := $(SHADER_SRCS:%.cpp=$(SHADER_OBJDIR)/%.o)
SHADER_DEPS     := $(SHADER_OBJS:.o=.d)

# Use pkg-config for SDL2 includes/libs. Link GLAD + libGL + libdl on Ubuntu.
SHADER_CXXFLAGS := -std=c++17 -Wall -Wextra -g -MMD -MP \
                   $(shell pkg-config --cflags sdl2)
SHADER_LDFLAGS  := $(shell pkg-config --libs sdl2) -lglad -lGL -ldl

.PHONY: shader-test
shader-test: $(SHADER_TARGET)

$(SHADER_TARGET): $(SHADER_OBJS)
	$(CXX) -o $@ $^ $(SHADER_LDFLAGS)

$(SHADER_OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(SHADER_CXXFLAGS) -c $< -o $@

-include $(SHADER_DEPS)
