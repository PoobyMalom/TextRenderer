# Compiler / flags
CXX       := g++
CXXFLAGS  := -std=c++17 -Wall -g -O2 -D_THREAD_SAFE \
             -Iinclude -Iinclude/cdt $(shell pkg-config --cflags sdl3) \
             -MMD -MP

LDFLAGS       := $(shell pkg-config --libs sdl3)
RELEASE_FLAGS := -O2 -DNDEBUG

# Targets
TARGET      := main
TEST_TARGET := test_runner
DEMO_TARGET := demo
TRI_TARGET  := triangulation
BENCH_TARGET := benchmark_cdt

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

TRI_MAIN_OBJ    := $(OBJDIR)/triangulation.o
TRI_MAIN_DEP    := $(TRI_MAIN_OBJ:.o=.d)

DEMO_MAIN_OBJ   := $(OBJDIR)/demo.o
DEMO_MAIN_DEP   := $(DEMO_MAIN_OBJ:.o=.d)
DEMO_EXTRA_SRCS := src/DemoPhases.cpp
DEMO_EXTRA_OBJS := $(DEMO_EXTRA_SRCS:%.cpp=$(OBJDIR)/%.o)
DEMO_EXTRA_DEPS := $(DEMO_EXTRA_OBJS:.o=.d)

# Constrained Delaunay triangulation library (vendored from ~/DelaunayTest) --
# only triangulation.cpp uses it so far, so it's linked into that target only.
CDT_SRCS := src/cdt/circle.cpp src/cdt/triangle.cpp src/cdt/triangulate.cpp \
            src/cdt/contour.cpp src/cdt/geometry.cpp src/cdt/constrain.cpp src/cdt/draw.cpp
CDT_OBJS := $(CDT_SRCS:%.cpp=$(OBJDIR)/%.o)
CDT_DEPS := $(CDT_OBJS:.o=.d)

BENCH_MAIN_OBJ := $(OBJDIR)/benchmark_cdt.o
BENCH_MAIN_DEP := $(BENCH_MAIN_OBJ:.o=.d)

.PHONY: all clean release run run-demo run-triangulation run-benchmark
all: $(TARGET)

$(TARGET): $(MAIN_OBJ) $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(TEST_TARGET): $(TEST_MAIN_OBJ) $(TEST_OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS) $(shell pkg-config --cflags --libs gtest_main)

$(DEMO_TARGET): $(DEMO_MAIN_OBJ) $(OBJS) $(DEMO_EXTRA_OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(TRI_TARGET): $(TRI_MAIN_OBJ) $(OBJS) $(CDT_OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(BENCH_TARGET): $(BENCH_MAIN_OBJ) $(OBJS) $(CDT_OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

release: CXXFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)

run: $(TARGET)
	./$(TARGET)

run-demo: $(DEMO_TARGET)
	./$(DEMO_TARGET)

run-triangulation: $(TRI_TARGET)
	./$(TRI_TARGET) src/fonts/JetBrainsMono-Regular.ttf

run-benchmark: $(BENCH_TARGET)
	./$(BENCH_TARGET)

clean:
	rm -rf $(OBJDIR) $(TARGET) $(TEST_TARGET) $(DEMO_TARGET) $(TRI_TARGET) $(BENCH_TARGET)

# Include auto-generated dependency files (ok if missing)
-include $(DEPS) $(MAIN_DEP) $(TEST_DEPS) $(TEST_MAIN_DEP) $(DEMO_MAIN_DEP) $(DEMO_EXTRA_DEPS) $(TRI_MAIN_DEP) $(CDT_DEPS) $(BENCH_MAIN_DEP)
