# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -Ignc

# Source files: main.cpp and all .cpp files in the gnc folder
SRC = main.cpp $(wildcard gnc/*.cpp)
OBJ = $(SRC:.cpp=.o)

# Output executable
TARGET = filter_test

# Default target
all: $(TARGET)

# Linking: combine object files to create the executable
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

# Compilation: compile .cpp files into .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up object files and the executable
clean:
	rm -f $(OBJ) $(TARGET)
