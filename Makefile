# SportButton Test Makefile
# Compile and run the desktop validation suite before deploying to an ESP32.

CXX      ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -DDESKTOP_TEST -I.

TEST_TARGET     = test_runner
PROTOCOL_TARGET = protocol_test

.PHONY: all test protocol clean install-deps arduino-check validate

# Default target
all: $(TEST_TARGET) $(PROTOCOL_TARGET)

# Desktop validation suite (all frames in can_test_data.h)
$(TEST_TARGET): test_runner.cpp can_messages.h can_test_data.h
	$(CXX) $(CXXFLAGS) -o $@ test_runner.cpp

# Protocol behaviour walkthrough (the 5 real-world cases)
$(PROTOCOL_TARGET): protocol_test.cpp can_messages.h
	$(CXX) $(CXXFLAGS) -o $@ protocol_test.cpp

# Run the validation suite
test: $(TEST_TARGET)
	./$(TEST_TARGET)

# Run the protocol walkthrough
protocol: $(PROTOCOL_TARGET)
	./$(PROTOCOL_TARGET)

# Clean build artifacts
clean:
	rm -f $(TEST_TARGET) $(PROTOCOL_TARGET)

install-deps:
	@echo "No dependencies required for desktop testing (needs g++)"

# Arduino IDE compilation check (requires arduino-cli and the ESP32 core)
arduino-check:
	@if command -v arduino-cli > /dev/null; then \
		echo "Checking Arduino compilation..."; \
		arduino-cli compile --fqbn esp32:esp32:esp32 .; \
	else \
		echo "arduino-cli not found. Install from: https://arduino.github.io/arduino-cli/"; \
	fi

# Full validation pipeline
validate: clean test arduino-check
	@echo ""
	@echo "========================================="
	@echo "    VALIDATION PIPELINE COMPLETE"
	@echo "========================================="
