CXX = g++
CXXFLAGS = -std=c++11 -Wall -I/opt/homebrew/include/SDL2 -D_THREAD_SAFE
LDFLAGS = -L/opt/homebrew/lib -lSDL2

TARGET = main
TEST_TARGET = test

SRCS = main.cpp src/MovablePoint.cpp src/Helpers.cpp src/TTFHeader.cpp src/TTFTable.cpp
OBJS = $(SRCS:.cpp=.o)

TEST_SRCS = test.cpp src/MovablePoint.cpp src/Helpers.cpp src/TTFHeader.cpp src/TTFTable.cpp src/HeadTable.cpp src/MaxpTable.cpp src/LocaTable.cpp src/CmapTable.cpp src/GlyphTable.cpp src/TTFFile.cpp
TEST_OBJS = $(TEST_SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_TARGET): $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -Iinclude -c $< -o $@

# Explicit rules for src directory
src/MovablePoint.o: src/MovablePoint.cpp include/MovablePoint.h
src/Helpers.o: src/Helpers.cpp include/Helpers.h
src/TTFHeader.o: src/TTFHeader.cpp include/TTFHeader.h
src/TTFTable.o: src/TTFTable.cpp include/TTFTable.h
src/HeadTable.o: src/HeadTable.cpp include/HeadTable.h
src/MaxpTable.o: src/MaxpTable.cpp include/MaxpTable.h
src/LocaTable.o: src/LocaTable.cpp include/LocaTable.h
src/CmapTable.o: src/CmapTable.cpp include/CmapTable.h
src/GlyphTable.o: src/GlyphTable.cpp include/GlyphTable.h

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(OBJS) $(TEST_OBJS)

.PHONY: all clean
