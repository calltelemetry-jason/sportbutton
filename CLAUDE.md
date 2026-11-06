# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Goal

Build a tiny, cheap in-car module for a 2008 BMW that listens to the 100 kbps K-CAN bus and, when it sees a specific CAN frame/byte, momentarily shorts two existing wires (like pressing a button).

## Implementation Strategy

**Components:**
- **Brains**: ESP32-WROOM-32 DevKit (uses built-in CAN/TWAI peripheral)
- **Bus interface**: SN65HVD230 (3.3V) → CANH/CANL on K-CAN, 120Ω jumper open
- **Output**: Simple 5V relay module (KY-019); drive from ESP32 GPIO; use COM+NO as dry contact
- **Power**: Car IGN 12V → 2A fuse → 12→5V buck (USB-C potted or Mini-360 set to 5.00V) → ESP32

**Form factor**: Compact, stacked boards, Dupont jumpers (no breadboard), 3D-printed case

**Success criteria**: Reliable CAN detect → relay pulses ~100–200ms → looks clean in car, uses easy Amazon/eBay parts, safely fused

## Hardware Architecture

The system consists of three main components:
- **ESP32-WROOM-32**: Main controller running Arduino firmware
- **SN65HVD230**: 3.3V CAN transceiver for K-CAN communication (100 kbps)
- **5V Relay Module**: KY-019 style relay for switching car circuits

Key hardware constraints:
- SN65HVD230 operates at 3.3V only (never apply 5V)
- 120Ω terminator on CAN transceiver must remain OPEN
- Relay module is active-HIGH (GPIO16 HIGH = energize)
- K-CAN runs at 100 kbps (not other BMW bus speeds)

## GPIO Pin Assignments

Fixed assignments for CAN peripheral:
- GPIO4: CAN RX (hardware fixed)
- GPIO5: CAN TX (hardware fixed)
- GPIO16: Relay control (safe 3.3V tolerant pin)

Alternative relay pins if needed: GPIO17, GPIO26, GPIO27

## Development Environment

This project uses Arduino IDE or PlatformIO for ESP32 development:

### Arduino IDE Setup
```bash
# Install ESP32 board package via Board Manager
# URL: https://dl.espressif.com/dl/package_esp32_index.json
# Board: "ESP32 Dev Module"
# Upload Speed: 921600
# CPU Frequency: 240MHz (WiFi/BT)
```

### PlatformIO Setup
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    ESP32CAN
```

## Code Structure

Expected file organization:
- `sportbutton.ino` - Main Arduino sketch with setup() and loop()
- `can_messages.h` - CAN message definitions and target byte patterns
- `hardware_design.md` - Complete hardware documentation and schematics

## CAN Bus Implementation

The project monitors BMW K-CAN bus for specific message patterns:
- Bus speed: 100 kbps
- Message format: Standard CAN frames
- Target detection: Specific byte patterns in CAN messages
- Response: Relay activation when pattern matches

## Safety Considerations

- Always disconnect car battery negative before wiring
- Use 2A fuse at 12V input for protection
- Test all circuits on bench before car installation
- Verify all grounds are common (buck, ESP32, relay, CAN transceiver)
- Never apply voltage to relay contacts (dry contact switching only)

## Testing and Validation

Before car installation:
1. Bench test CAN message reception
2. Verify relay activation on target messages (GPIO16 HIGH = relay energized)
3. Confirm power supply stability (5.00V ±0.1V)
4. Test GPIO16 logic levels (3.3V HIGH = relay ON, 0V LOW = relay OFF)

## Hardware Dependencies

All components must be compatible with automotive 12V systems and temperature ranges. The buck converter must provide clean 5V power to prevent ESP32 reset issues during relay switching.