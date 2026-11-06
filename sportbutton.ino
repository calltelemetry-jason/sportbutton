/*
 * SportButton - BMW Diamond Button CAN Listener
 * Listens for CAN ID 0x1D6 with payload C040 and triggers relay
 * Hardware: ESP32-WROOM-32 + SN65HVD230 + 5V Relay Module
 */

#include "driver/twai.h"
#include "can_messages.h"
#include "can_test_data.h"

// Global variables
can_stats_t stats = {0};
bool relay_active = false;
unsigned long relay_start_time = 0;
unsigned long last_button_time = 0;
bool test_mode = false;

void setup() {
    Serial.begin(115200);
    Serial.println("SportButton - BMW Diamond Button Listener");
    Serial.println("Hardware: ESP32 + SN65HVD230 + Relay");
    
    // Initialize GPIO pins
    initializeGPIO();
    
    // Initialize CAN bus
    if (!initializeCAN()) {
        Serial.println("ERROR: CAN initialization failed!");
        stats.last_error = ERR_CAN_INIT_FAILED;
        blinkError();
        return;
    }
    
    Serial.println("System ready - Listening for Diamond Button (0x1D6:C040)");
    Serial.println("Commands: 't' = run tests, 's' = show stats, 'r' = reset stats");
    
    // Blink status LED to indicate ready
    blinkStatusLED(3);
}

void loop() {
    // Handle serial commands
    handleSerialCommands();
    
    // Check for CAN messages
    twai_message_t message;
    if (twai_receive(&message, 0) == ESP_OK) {
        stats.total_frames++;
        processCANMessage(&message);
    }
    
    // Handle relay timing
    updateRelayState();
    
    // Periodic status update every 10 seconds
    static unsigned long last_status = 0;
    if (millis() - last_status > 10000) {
        printStatus();
        last_status = millis();
    }
    
    delay(1); // Small delay to prevent watchdog issues
}

void initializeGPIO() {
    // Relay control pin
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW); // Relay OFF
    
    // Status LED
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);
    
    Serial.println("GPIO initialized");
}

bool initializeCAN() {
    // Configure CAN timing for 100 kbps
    twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_100KBITS();
    
    // CRITICAL FIX: Hardware filter for ONLY 0x1D6 messages
    // This prevents message loss on busy BMW K-CAN bus
    twai_filter_config_t filter_config = {
        .acceptance_code = (0x1D6 << 21),  // Accept only ID 0x1D6
        .acceptance_mask = ~(TWAI_STD_ID_MASK << 21),  // Standard 11-bit frame mask
        .single_filter = true
    };
    
    // Configure CAN general settings
    twai_general_config_t general_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)CAN_TX_PIN, 
        (gpio_num_t)CAN_RX_PIN, 
        TWAI_MODE_LISTEN_ONLY
    );
    
    // Install TWAI driver
    esp_err_t result = twai_driver_install(&general_config, &timing_config, &filter_config);
    if (result != ESP_OK) {
        Serial.printf("TWAI driver install failed: %s\n", esp_err_to_name(result));
        return false;
    }
    
    // Start TWAI driver
    result = twai_start();
    if (result != ESP_OK) {
        Serial.printf("TWAI start failed: %s\n", esp_err_to_name(result));
        stats.last_error = ERR_CAN_START_FAILED;
        return false;
    }
    
    Serial.println("CAN bus initialized at 100 kbps");
    return true;
}

void processCANMessage(twai_message_t* msg) {
    // Due to hardware filtering, we ONLY receive 0x1D6 messages
    // Validate frame format and check for Diamond Button payload
    if (!(msg->flags & TWAI_MSG_FLAG_RTR) && msg->data_length_code >= 2) {
        
        // Debug output for all 0x1D6 messages in test mode
        if (test_mode) {
            Serial.printf("RX 0x1D6: ");
            Serial.printf(DEBUG_CAN_FORMAT, DEBUG_CAN_ARGS(msg->identifier, msg->data_length_code, msg->data));
            Serial.println();
        }
        
        // Check for Diamond Button press (C040)
        if (msg->data[0] == DIAMOND_BUTTON_BYTE0 && msg->data[1] == DIAMOND_BUTTON_BYTE1) {
            unsigned long current_time = millis();
            
            // Debounce check (button sends multiple C040 messages)
            if (current_time - last_button_time < DEBOUNCE_TIME_MS) {
                return; // Too soon, ignore repeated presses
            }
            
            last_button_time = current_time;
            stats.diamond_button_frames++;
            
            Serial.printf("🎯 DIAMOND BUTTON PRESSED! Data:%02X %02X (frame #%lu)\n", 
                         msg->data[0], msg->data[1], stats.diamond_button_frames);
            
            activateRelay();
            blinkStatusLED(1);
        }
        // Log other button states if in debug mode
        else if (test_mode) {
            if (msg->data[0] == 0xC0 && msg->data[1] == 0x10) {
                Serial.println("📱 Other button: C010");
            } else if (msg->data[0] == 0xC1 && msg->data[1] == 0x00) {
                Serial.println("📱 Other button: C100");  
            } else if (msg->data[0] == 0xC0 && msg->data[1] == 0x01) {
                Serial.println("📱 Other button: C001");
            } else if (msg->data[0] == 0xC0 && msg->data[1] == 0x00) {
                Serial.println("⚪ Normal state: C000 (no buttons pressed)");
            }
        }
    }
}

void activateRelay() {
    if (relay_active) {
        return; // Already active
    }
    
    relay_active = true;
    relay_start_time = millis();
    stats.relay_activations++;
    stats.last_activation_ms = relay_start_time;
    
    // Activate relay (active-low logic)
    digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? LOW : HIGH);
    
    Serial.printf("RELAY ACTIVATED (pulse #%lu)\n", stats.relay_activations);
}

void updateRelayState() {
    if (relay_active) {
        if (millis() - relay_start_time >= RELAY_PULSE_MS) {
            // Deactivate relay
            digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
            relay_active = false;
            Serial.println("RELAY DEACTIVATED");
        }
    }
}

void handleSerialCommands() {
    if (Serial.available()) {
        char cmd = Serial.read();
        switch (cmd) {
            case 't':
            case 'T':
                runTestSuite();
                break;
            case 's':
            case 'S':
                printDetailedStats();
                break;
            case 'r':
            case 'R':
                resetStats();
                break;
            case 'd':
            case 'D':
                test_mode = !test_mode;
                Serial.printf("Debug mode: %s\n", test_mode ? "ON" : "OFF");
                break;
            case 'h':
            case 'H':
                printHelp();
                break;
        }
    }
}

void printStatus() {
    Serial.printf("Status: Frames:%lu Diamond:%lu Relays:%lu Uptime:%lus\n", 
                  stats.total_frames, stats.diamond_button_frames, 
                  stats.relay_activations, millis()/1000);
}

void printDetailedStats() {
    Serial.println("\n=== SportButton Statistics ===");
    Serial.printf("Total CAN frames received: %lu\n", stats.total_frames);
    Serial.printf("Diamond Button detections: %lu\n", stats.diamond_button_frames);
    Serial.printf("Relay activations: %lu\n", stats.relay_activations);
    Serial.printf("Last activation: %lu ms ago\n", 
                  stats.last_activation_ms > 0 ? millis() - stats.last_activation_ms : 0);
    Serial.printf("Uptime: %lu seconds\n", millis()/1000);
    Serial.printf("Relay state: %s\n", relay_active ? "ACTIVE" : "INACTIVE");
    Serial.printf("Last error: %d\n", stats.last_error);
    Serial.println("==============================\n");
}

void resetStats() {
    stats.total_frames = 0;
    stats.diamond_button_frames = 0;
    stats.relay_activations = 0;
    stats.last_activation_ms = 0;
    stats.last_error = ERR_NONE;
    Serial.println("Statistics reset");
}

void printHelp() {
    Serial.println("\n=== SportButton Commands ===");
    Serial.println("t - Run test suite");
    Serial.println("s - Show detailed statistics");
    Serial.println("r - Reset statistics");
    Serial.println("d - Toggle debug mode");
    Serial.println("h - Show this help");
    Serial.println("============================\n");
}

void blinkStatusLED(int count) {
    for (int i = 0; i < count; i++) {
        digitalWrite(STATUS_LED_PIN, HIGH);
        delay(LED_BLINK_MS);
        digitalWrite(STATUS_LED_PIN, LOW);
        if (i < count - 1) delay(LED_BLINK_MS);
    }
}

void blinkError() {
    // Rapid blinking for error indication
    for (int i = 0; i < 10; i++) {
        digitalWrite(STATUS_LED_PIN, HIGH);
        delay(50);
        digitalWrite(STATUS_LED_PIN, LOW);
        delay(50);
    }
}

// ===== TEST SUITE IMPLEMENTATION =====

void runTestSuite() {
    Serial.println("\n========================================");
    Serial.println("      SPORTBUTTON TEST SUITE");
    Serial.println("========================================");
    
    test_results_t results = {0};
    bool old_test_mode = test_mode;
    test_mode = true; // Enable debug output during tests
    
    Serial.printf("Running %d test cases...\n\n", NUM_TEST_FRAMES);
    
    for (int i = 0; i < NUM_TEST_FRAMES; i++) {
        bool test_passed = runSingleTest(&test_frames[i], &results, i + 1);
        if (!test_passed) {
            Serial.printf("❌ Test %d FAILED\n", i + 1);
        } else {
            Serial.printf("✅ Test %d PASSED\n", i + 1);
        }
    }
    
    // Summary
    Serial.println("\n========================================");
    Serial.println("           TEST RESULTS");
    Serial.println("========================================");
    Serial.printf("Total tests run: %d\n", NUM_TEST_FRAMES);
    Serial.printf("Tests passed: %d\n", NUM_TEST_FRAMES - results.false_positives - results.false_negatives);
    Serial.printf("Tests failed: %d\n", results.false_positives + results.false_negatives);
    Serial.printf("False positives: %lu\n", results.false_positives);
    Serial.printf("False negatives: %lu\n", results.false_negatives);
    
    if (results.false_positives == 0 && results.false_negatives == 0) {
        Serial.println("🎉 ALL TESTS PASSED! 🎉");
        results.test_passed = true;
        blinkStatusLED(5); // Victory blinks
    } else {
        Serial.println("❌ SOME TESTS FAILED ❌");
        results.test_passed = false;
        blinkError();
    }
    
    Serial.println("========================================\n");
    
    test_mode = old_test_mode; // Restore previous debug mode
}

bool runSingleTest(const can_test_frame_t* test_frame, test_results_t* results, int test_num) {
    Serial.printf("Test %d: %s\n", test_num, test_frame->description);
    Serial.printf("  CAN ID: 0x%03X, DLC: %d, Data: ", test_frame->can_id, test_frame->dlc);
    
    for (int i = 0; i < test_frame->dlc; i++) {
        Serial.printf("%02X ", test_frame->data[i]);
    }
    Serial.println();
    
    // Record initial relay activation count
    uint32_t initial_activations = stats.relay_activations;
    
    // Create a TWAI message from test data
    twai_message_t test_msg = {0};
    test_msg.identifier = test_frame->can_id;
    test_msg.data_length_code = test_frame->dlc;
    test_msg.flags = TWAI_MSG_FLAG_NONE;
    
    for (int i = 0; i < test_frame->dlc && i < 8; i++) {
        test_msg.data[i] = test_frame->data[i];
    }
    
    // Process the test message
    processCANMessage(&test_msg);
    
    // Wait for relay timing to complete if activated
    delay(RELAY_PULSE_MS + 10);
    updateRelayState(); // Make sure relay state is updated
    
    // Check if relay was activated
    bool relay_was_activated = (stats.relay_activations > initial_activations);
    
    Serial.printf("  Expected trigger: %s, Actual trigger: %s\n", 
                  test_frame->should_trigger ? "YES" : "NO",
                  relay_was_activated ? "YES" : "NO");
    
    // Evaluate test result
    bool test_passed = (test_frame->should_trigger == relay_was_activated);
    
    if (!test_passed) {
        if (test_frame->should_trigger && !relay_was_activated) {
            results->false_negatives++;
            Serial.println("  ❌ FALSE NEGATIVE - Should have triggered but didn't");
        } else if (!test_frame->should_trigger && relay_was_activated) {
            results->false_positives++;
            Serial.println("  ❌ FALSE POSITIVE - Triggered when it shouldn't");
        }
    } else {
        Serial.println("  ✅ CORRECT");
    }
    
    results->frames_received++;
    if (relay_was_activated) {
        results->relay_activations++;
    }
    if (test_frame->should_trigger) {
        results->target_frames_detected++;
    }
    
    Serial.println();
    
    return test_passed;
}