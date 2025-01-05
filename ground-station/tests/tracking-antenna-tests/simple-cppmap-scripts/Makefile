# Compiler to use
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

# Include directories
# Assuming cppmap3d headers are in the cppmap3d/ directory
INCLUDES = -I../cppmap3d

# Source files
SRCS = cppmap_test.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Executable name
TARGET = main.exe

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Compile source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean up build files
clean:
	rm -f $(TARGET) $(OBJS)

# Phony targets
.PHONY: all clean
