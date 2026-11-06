/*
 * CAN Test Data for SportButton
 * BMW K-CAN Bus Testing and Validation
 * Target: CAN ID 0x1D6, Diamond Button payload C040
 */

#ifndef CAN_TEST_DATA_H
#define CAN_TEST_DATA_H

// Target CAN message configuration
#define TARGET_CAN_ID           0x1D6    // BMW Diamond Button CAN ID
#define TARGET_PAYLOAD_BYTE0    0xC0     // First byte of Diamond Button payload
#define TARGET_PAYLOAD_BYTE1    0x40     // Second byte of Diamond Button payload
#define RELAY_PULSE_DURATION    150      // ms - relay activation time

// Test data structures
typedef struct {
    uint32_t can_id;
    uint8_t dlc;           // Data Length Code (0-8 bytes)
    uint8_t data[8];       // CAN frame data
    bool should_trigger;   // Expected relay trigger result
    const char* description;
} can_test_frame_t;

// Comprehensive test dataset
const can_test_frame_t test_frames[] = {
    // POSITIVE TESTS - Should trigger relay
    {
        .can_id = 0x1D6,
        .dlc = 2,
        .data = {0xC0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .should_trigger = true,
        .description = "Diamond Button - Exact match C040"
    },
    {
        .can_id = 0x1D6,
        .dlc = 8,
        .data = {0xC0, 0x40, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        .should_trigger = true,
        .description = "Diamond Button - C040 with extra data"
    },
    {
        .can_id = 0x1D6,
        .dlc = 3,
        .data = {0xC0, 0x40, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00},
        .should_trigger = true,
        .description = "Diamond Button - C040 with 3 byte frame"
    },

    // NEGATIVE TESTS - Should NOT trigger relay
    {
        .can_id = 0x1D6,
        .dlc = 2,
        .data = {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .should_trigger = false,
        .description = "Normal state - C000 (no buttons pressed)"
    },
    {
        .can_id = 0x1D6,
        .dlc = 2,
        .data = {0xC0, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .should_trigger = false,
        .description = "Other button - C010"
    },
    {
        .can_id = 0x1D6,
        .dlc = 2,
        .data = {0xC1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .should_trigger = false,
        .description = "Other button - C100"
    },
    {
        .can_id = 0x1D6,
        .dlc = 2,
        .data = {0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .should_trigger = false,
        .description = "Other button - C001"
    },
    {
        .can_id = 0x1D6,
        .dlc = 2,
        .data = {0xC0, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .should_trigger = false,
        .description = "Wrong second byte - C041"
    },
    {
        .can_id = 0x1D6,
        .dlc = 2,
        .data = {0xC1, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .should_trigger = false,
        .description = "Wrong first byte - C140"
    },
    {
        .can_id = 0x1D6,
        .dlc = 1,
        .data = {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .should_trigger = false,
        .description = "Incomplete frame - only C0"
    },
    {
        .can_id = 0x1D5,
        .dlc = 2,
        .data = {0xC0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .should_trigger = false,
        .description = "Wrong CAN ID - 0x1D5"
    },
    {
        .can_id = 0x1D7,
        .dlc = 2,
        .data = {0xC0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .should_trigger = false,
        .description = "Wrong CAN ID - 0x1D7"
    },
    {
        .can_id = 0x1D6,
        .dlc = 0,
        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .should_trigger = false,
        .description = "Empty frame"
    },

    // EDGE CASES
    {
        .can_id = 0x1D6,
        .dlc = 8,
        .data = {0xC0, 0x40, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},
        .should_trigger = true,
        .description = "Full 8-byte frame with C040 prefix"
    },
    {
        .can_id = 0x1D6,
        .dlc = 2,
        .data = {0x40, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .should_trigger = false,
        .description = "Reversed bytes - 40C0"
    }
};

#define NUM_TEST_FRAMES (sizeof(test_frames) / sizeof(can_test_frame_t))

// Additional test constants for validation
#define BMW_K_CAN_SPEED         100000   // 100 kbps
#define EXPECTED_FRAME_RATE     50       // Approximate frames per second on K-CAN
#define TEST_TIMEOUT_MS         5000     // Max time to wait for test completion

// Common BMW CAN IDs for reference (should NOT trigger)
#define BMW_ENGINE_RPM_ID       0x0AA    // Engine RPM
#define BMW_VEHICLE_SPEED_ID    0x0CE    // Vehicle speed
#define BMW_STEERING_ANGLE_ID   0x0C4    // Steering wheel angle
#define BMW_BRAKE_STATUS_ID     0x34F    // Brake pedal status

// Validation macros
#define IS_TARGET_FRAME(id, data) \
    ((id == TARGET_CAN_ID) && \
     (data[0] == TARGET_PAYLOAD_BYTE0) && \
     (data[1] == TARGET_PAYLOAD_BYTE1))

#define VALIDATE_CAN_ID(id) \
    ((id >= 0x000) && (id <= 0x7FF))  // Standard CAN ID range

// Test result structure
typedef struct {
    uint32_t frames_received;
    uint32_t target_frames_detected;
    uint32_t relay_activations;
    uint32_t false_positives;
    uint32_t false_negatives;
    bool test_passed;
} test_results_t;

#endif // CAN_TEST_DATA_H