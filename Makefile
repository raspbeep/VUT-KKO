CXX = g++
CXXFLAGS = -Wall -Wextra -O1 -std=c++23 -Iinclude -march=native

# Source files (only .cpp files)
SRCS = src/argparser.cpp src/linkedlist.cpp src/block.cpp src/hashtable.cpp src/block_reader.cpp src/block_writer.cpp src/lz_codec.cpp

# Object files
OBJS = $(addprefix $(BUILD_DIR)/,$(notdir $(SRCS:.cpp=.o)))

# Executable name
TARGET_NAME = lz_codec
TARGET = $(BUILD_DIR)/$(TARGET_NAME)

BUILD_DIR = build

.PHONY: all
all: $(TARGET)

# Compile object files
$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	@echo "Compiling $< -> $@"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link the object files to create the executable
$(TARGET): $(OBJS)
	@echo "Linking object files -> $(TARGET)"
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

.PHONY: run
run: $(TARGET)
	@echo "Running $(TARGET)..."
	./$(TARGET) -c

.PHONY: clean
clean:
	@echo "Cleaning up..."
	rm -rf $(BUILD_DIR)
	@echo "Cleanup complete."
