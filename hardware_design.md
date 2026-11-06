# SportButton Hardware Design
**BMW K-CAN Listener with Relay Control**

## Overview
Listen to BMW K-CAN (100 kbps) with ESP32-WROOM-32 and SN65HVD230. When a chosen CAN byte is detected, short two wires in the car using a 5V relay module. System powered from car 12V via buck converter.

## Bill of Materials

| Component | Part Number/Type | Notes |
|-----------|------------------|-------|
| ESP32 DevKit | ESP32-WROOM-32/ESP-32S | USB-C or micro-USB, CH340C/CP2102 |
| CAN Transceiver | SN65HVD230 3.3V module | Leave 120Ω terminator OPEN |
| Relay Module | KY-019 style 5V relay | Pins: S/IN, –/GND, +/VCC (HIGH trigger) |
| Buck Converter | 12V→5V 3A | Potted USB-C or Mini-360 |
| Fuse | 2A blade fuse + holder | Inline at 12V feed |
| Wiring | Dupont female-female jumpers | Twisted pair for CANH/CANL |

## Pin Mapping

### SN65HVD230 ↔ ESP32
```
SN65HVD230    ESP32
----------    -----
3.3V     →    3V3
GND      →    GND
CTX      →    GPIO5 (CAN-TX)
CRX      →    GPIO4 (CAN-RX)
CANH     →    K-CAN High (twisted pair)
CANL     →    K-CAN Low  (twisted pair)
```

### Relay Module ↔ ESP32
```
Relay Module    ESP32
------------    -----
+ (VCC)    →    5V/Vin
– (GND)    →    GND
S (IN)     →    GPIO16
```

### Relay Contacts ↔ Car
```
Relay Terminal    Connection
--------------    ----------
COM          →    Target wire 1
NO           →    Target wire 2
NC           →    (unused)
```

## Power Distribution

```
Switched 12V (IGN) → [2A Fuse] → Buck Converter (12V→5V)
                                       ↓
Buck 5V OUT → ESP32 USB-C/5V pin → Relay Module VCC
                                       ↓
All GND connections tied to chassis ground
```

## Schematic

```
     Switched 12V (IGN)            Car chassis
            │                           │
        [2A Fuse]                       │
            │                           │
      +-----┴------+                    │
      |  12→5V Buck |                   │
      +-----┬------+                    │
            │5V                         │GND
            │                           │
     ┌──────┴───────────────┐           │
     │   ESP32 DevKit       │           │
     │  (ESP32-WROOM-32)    │           │
     │                      │           │
     │ 5V/Vin  ◄────────────┘           └──────────────┐
     │ GND     ◄────────────────────────────────────────┘
     │ 3V3  ──────────────────┐
     │ GND  ──────────────────┼─────────────────────────┐
     │ GPIO5 (CAN_TX) ────────┼──┐                      │
     │ GPIO4 (CAN_RX) ────────┼──┼──┐                   │
     │ GPIO16 (Relay IN) ─────┼──┼──┼──┐                │
     └───────────────────────┼──┼──┼──┼─────────────────┘
                             │  │  │  │
      ┌─────────────────────┼──┼──┘  │
      │ SN65HVD230          │  │     │
      │ (3.3V transceiver)  │  │     │
      │ 3.3V  ◄─────────────┘  │     │
      │ GND   ◄────────────────┼─────┼─────────────────┐
      │ CTX   ◄────────────────┘     │                 │
      │ CRX   ◄──────────────────────┘                 │
      │ CANH ────────────────────────→ K-CAN High      │
      │ CANL ────────────────────────→ K-CAN Low       │
      │ [120Ω jumper OPEN]           │                 │
      └──────────────────────────────┘                 │
                                                       │
      ┌─────────────────────────────────────────────────┼──┐
      │ 5V Relay Module                                 │  │
      │ +/VCC ◄─────────────────────────────────────────┘  │
      │ –/GND ◄────────────────────────────────────────────┘
      │ S/IN  ◄────────────────────────────────────────────┘
      │                                                    │
      │ Relay Contacts (screw terminals):                  │
      │ COM ──────────────┐                                │
      │ NO  ──────────────┴─→ Across target car wires      │
      │ NC  (unused)                                       │
      └────────────────────────────────────────────────────┘
```

## Build Instructions

### Step 1: Power System
1. Install buck converter in accessible location
2. Connect IGN 12V → 2A fuse → buck converter positive input
3. Connect buck converter negative to chassis ground
4. Set buck output to exactly 5.00V (if using adjustable Mini-360)

### Step 2: ESP32 Power
1. Connect buck 5V output to ESP32 via USB-C cable OR directly to 5V/Vin pin
2. Verify ESP32 powers up and can be programmed

### Step 3: CAN Interface
1. Wire SN65HVD230 to ESP32 per pin mapping above
2. **IMPORTANT**: Leave 120Ω terminator jumper OPEN
3. Connect CANH/CANL to K-CAN using twisted pair wire
4. Test CAN reception before proceeding

### Step 4: Relay Control
1. Wire relay module control pins (VCC/GND/IN) to ESP32
2. Connect relay contacts (COM/NO) across target car wires
3. Test relay operation: GPIO16 HIGH should energize relay

### Step 5: Integration Test
1. Bench test complete system before car installation
2. Verify CAN message reception
3. Verify relay activation on target CAN message
4. Check all power connections and grounds

## Critical Notes

### Power
- **Never** apply 5V to SN65HVD230 (3.3V only)
- Ensure all grounds are common (buck, ESP32, relay, CAN transceiver)
- Use only one power source to ESP32 during operation

### CAN Bus
- Keep CANH/CANL wires twisted and as short as practical
- 120Ω terminator must remain OPEN on SN65HVD230
- K-CAN runs at 100 kbps

### Relay
- This KY-019 module is active-HIGH (GPIO16 HIGH = energize)
- Relay contacts are dry contact only - no voltage applied
- Test relay click before final installation

### Safety
- Always use 2A fuse at 12V input
- Disconnect battery negative when making connections
- Verify all connections before applying power
- Test on bench before car installation

## GPIO Pin Assignments

```
GPIO4  - CAN RX (fixed - CAN peripheral)
GPIO5  - CAN TX (fixed - CAN peripheral)  
GPIO16 - Relay control (safe choice, 3.3V tolerant)
```

Alternative safe relay control pins: GPIO17, GPIO26, GPIO27

## Troubleshooting

### No CAN Messages
- Check CANH/CANL connections and polarity
- Verify 120Ω terminator is OPEN
- Confirm K-CAN is 100 kbps, not other BMW buses

### Relay Not Working
- Verify 5V supply to relay module
- Check GPIO16 logic level (should be 3.3V when HIGH for activation)
- Test relay with direct 5V/GND application to S/IN pin
- This relay energizes when GPIO16 is HIGH (not LOW)

### Power Issues
- Check fuse continuity
- Verify buck converter output voltage (should be 5.00V ±0.1V)
- Ensure all grounds are connected

## Files Generated
- `hardware_design.md` - This comprehensive design document
- Ready for: `sportbutton.ino` - Arduino code for ESP32
- Ready for: `can_messages.h` - CAN message definitions