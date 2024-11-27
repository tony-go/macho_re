# Build directory
BUILD_DIR = build

# Default target
all:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. && cmake --build .

.PHONY: test
test: all
	@./$(BUILD_DIR)/macho_helper_cli /bin/ls

# Clean build directory
.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
