CXX = g++
CXXFLAGS = -std=c++11 -Wall -I/opt/homebrew/include/SDL2 -D_THREAD_SAFE -g
LDFLAGS = -L/opt/homebrew/lib -lSDL2

TARGET = main
TEST_TARGET = test

SRCS = main.cpp src/MovablePoint.cpp src/Helpers.cpp src/TTFHeader.cpp src/TTFTable.cpp
OBJS = $(SRCS:.cpp=.o)

TEST_SRCS = test.cpp src/MovablePoint.cpp src/Helpers.cpp src/TTFHeader.cpp src/TTFTable.cpp src/HeadTable.cpp src/MaxpTable.cpp src/LocaTable.cpp src/CmapTable.cpp src/GlyphTable.cpp src/TTFFile.cpp src/SDLInitializer.cpp
TEST_OBJS = $(TEST_SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_TARGET): $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	rm -f $(TEST_OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -Iinclude -c $< -o $@

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(OBJS) $(TEST_OBJS)

.PHONY: all clean
