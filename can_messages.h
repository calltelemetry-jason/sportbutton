/*
 * BMW CAN Message Definitions - SportButton Diamond Button Detection
 * 
 * CONFIRMED FROM REAL BMW 2008 K-CAN DATA:
 * Target: CAN ID 0x1D6, payload C040 (Diamond Button press)
 * Normal: CAN ID 0x1D6, payload C000 (no buttons pressed - sent ~1Hz)
 * Other:  CAN ID 0x1D6, payload C010/C100/C001 (other steering wheel buttons)
 * 
 * PROTOCOL: No separate "release" message - button simply stops sending C040
 */

#ifndef CAN_MESSAGES_H
#define CAN_MESSAGES_H

#include <stdint.h>

// BMW K-CAN Bus Configuration
#define BMW_K_CAN_SPEED         100000   // 100 kbps
#define CAN_RX_PIN              4        // GPIO4 - Fixed for ESP32 CAN
#define CAN_TX_PIN              5        // GPIO5 - Fixed for ESP32 CAN

// Diamond Button Configuration
#define DIAMOND_BUTTON_CAN_ID   0x1D6    // BMW Diamond Button CAN ID
#define DIAMOND_BUTTON_BYTE0    0xC0     // First payload byte
#define DIAMOND_BUTTON_BYTE1    0x40     // Second payload byte
#define MIN_PAYLOAD_LENGTH      2        // Minimum bytes needed for detection

// Relay Control
#define RELAY_PIN               16       // GPIO16 - Relay control
#define RELAY_ACTIVE_LOW        false    // This relay triggers on HIGH
#define RELAY_PULSE_MS          150      // Relay activation duration (ms)

// Detection Logic
inline bool isDiamondButtonPressed(uint32_t can_id, const uint8_t* data, uint8_t dlc) {
    return (can_id == DIAMOND_BUTTON_CAN_ID) &&
           (dlc >= MIN_PAYLOAD_LENGTH) &&
           (data[0] == DIAMOND_BUTTON_BYTE0) &&
           (data[1] == DIAMOND_BUTTON_BYTE1);
}

// Debug output format
#define DEBUG_CAN_FORMAT "ID:0x%03X DLC:%d DATA:%02X %02X %02X %02X %02X %02X %02X %02X"
#define DEBUG_CAN_ARGS(id, dlc, data) id, dlc, \
    data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]

// Status LED (if available)
#define STATUS_LED_PIN          2        // GPIO2 - Built-in LED on most ESP32 boards
#define LED_BLINK_MS            100      // LED blink duration for activity indication

// Timing constants
#define CAN_INIT_TIMEOUT_MS     5000     // Max time to wait for CAN init
#define DEBOUNCE_TIME_MS        50       // Minimum time between button presses
#define WATCHDOG_TIMEOUT_MS     30000    // Watchdog timer (30 seconds)

// Error codes
typedef enum {
    ERR_NONE = 0,
    ERR_CAN_INIT_FAILED,
    ERR_CAN_START_FAILED,
    ERR_RELAY_INIT_FAILED,
    ERR_WATCHDOG_TIMEOUT
} error_code_t;

// Frame statistics
typedef struct {
    uint32_t total_frames;
    uint32_t diamond_button_frames;
    uint32_t relay_activations;
    uint32_t last_activation_ms;
    error_code_t last_error;
} can_stats_t;

#endif // CAN_MESSAGES_H