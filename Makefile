# Build directory
BUILD_DIR = build
BUILD_TYPE = Debug

# Default target
all:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) .. && cmake --build .

.PHONY: test
test: all
	ctest --test-dir $(BUILD_DIR) --output-on-failure


.PHONY: run
run: all
	@./$(BUILD_DIR)/macho_re /bin/ls

# Clean build directory
.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
