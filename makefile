# Compiler / flags
CXX       := g++
CXXFLAGS  := -std=c++17 -Wall -g -D_THREAD_SAFE \
             -Iinclude $(shell sdl2-config --cflags) \
             -MMD -MP

LDFLAGS       := $(shell sdl2-config --libs)
RELEASE_FLAGS := -O2 -DNDEBUG

# Targets
TARGET      := main
TEST_TARGET := test_runner

# Build dir
OBJDIR := build

# Sources
SRCS := main.cpp \
        src/Helpers.cpp src/TTFHeader.cpp src/TTFTable.cpp \
        src/HeadTable.cpp src/MaxpTable.cpp src/LocaTable.cpp src/CmapTable.cpp \
        src/GlyphTable.cpp src/TTFFile.cpp src/SDLInitializer.cpp src/Renderer.cpp \
        src/Metrics.cpp

TEST_SRCS := tests/test_helpers.cpp \
             src/Helpers.cpp src/TTFHeader.cpp src/TTFTable.cpp \
             src/HeadTable.cpp src/MaxpTable.cpp src/LocaTable.cpp src/CmapTable.cpp \
             src/GlyphTable.cpp src/TTFFile.cpp src/SDLInitializer.cpp src/Renderer.cpp \
             src/Metrics.cpp

# All objects go under build/
MAIN_OBJ := $(OBJDIR)/main.o
MAIN_DEP := $(MAIN_OBJ:.o=.d)

NONMAIN_SRCS := $(filter-out main.cpp,$(SRCS))
OBJS := $(NONMAIN_SRCS:%.cpp=$(OBJDIR)/%.o)
DEPS := $(OBJS:.o=.d)

TEST_MAIN_OBJ := $(OBJDIR)/tests/test_helpers.o
TEST_MAIN_DEP := $(TEST_MAIN_OBJ:.o=.d)
TEST_OBJS     := $(OBJS)
TEST_DEPS     := $(DEPS)

.PHONY: all clean release run
all: $(TARGET)

$(TARGET): $(MAIN_OBJ) $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(TEST_TARGET): $(TEST_MAIN_OBJ) $(TEST_OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS) $(shell pkg-config --cflags --libs gtest_main)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

release: CXXFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(OBJDIR) $(TARGET) $(TEST_TARGET)

# Include auto-generated dependency files (ok if missing)
-include $(DEPS) $(MAIN_DEP) $(TEST_DEPS) $(TEST_MAIN_DEP)
