CXX = g++
CXXFLAGS = -Wall -Wextra -O0 -g -std=c++23 -Isrc -Iinclude -march=native

SRCS = src/transformations.cpp src/argparser.cpp src/image.cpp src/block.cpp src/hashtable.cpp src/block_reader.cpp src/block_writer.cpp src/lz_codec.cpp

OBJS = $(addprefix $(BUILD_DIR)/,$(notdir $(SRCS:.cpp=.o)))

TARGET_NAME = lz_codec
TARGET = $(BUILD_DIR)/$(TARGET_NAME)
ZIP_NAME = xkrato61.zip

BUILD_DIR = build

.PHONY: all run clean zip
all: $(TARGET)
	cp $(TARGET) ./$(TARGET_NAME)

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	@echo "Compiling $< -> $@"
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	@echo "Linking object files -> $(TARGET)"
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

run: $(TARGET)
	@echo "Running $(TARGET)..."
	./$(TARGET) -c

clean:
	@echo "Cleaning up..."
	rm -rf $(BUILD_DIR)
	rm -rf tmp/
	rm -f lz_codec
	rm -f $(ZIP_NAME)
	@echo "Cleanup complete."

zip:
	@echo "Creating zip archive $(ZIP_NAME)..."
	zip -r $(ZIP_NAME) src include Makefile report.pdf readme.md
	@echo "Zip archive created."
