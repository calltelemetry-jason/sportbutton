# SportButton

An ESP32 module that listens to a 2008 BMW **K-CAN** bus and pulses a relay when
the steering-wheel **Diamond Button** is pressed — so an aftermarket accessory can
be triggered from a factory button, without modifying the car's wiring harness.

The button press is detected as CAN ID `0x1D6` with payload `C0 40` at 100 kbps.
The relay is a dry contact across two existing wires, pulsed for 150 ms.

> **Status:** bench-validated. Not yet installed in a car. Read the
> [safety notes](#safety) before wiring anything into a vehicle.

## How it works

| Item | Value |
|------|-------|
| Bus | BMW K-CAN, 100 kbps, standard 11-bit frames |
| Trigger frame | `0x1D6` / `C0 40` (Diamond Button) |
| Neutral frame | `0x1D6` / `C0 00` (no button pressed, ~1 Hz) |
| Other buttons | `C0 10`, `C1 00`, `C0 01` — ignored |
| Release message | none; the button simply stops sending `C0 40` |
| Output | Active-HIGH relay pulse, GPIO16, 150 ms |
| Bus access | listen-only (TWAI `TWAI_MODE_LISTEN_ONLY`) |

A hardware acceptance filter passes only `0x1D6`, keeping the ESP32 off the rest
of the busy K-CAN traffic. Repeated `C0 40` frames while the button is held are
debounced with a 50 ms window so one press produces one relay pulse.

The full capture analysis is in [`can_analysis.md`](can_analysis.md); the raw
capture is [`trace2.candump`](trace2.candump) (93,398 frames, `candump -L` format).

## Hardware

Bill of materials and full schematics: [`hardware_design.md`](hardware_design.md).

| Part | Notes |
|------|-------|
| ESP32-WROOM-32 DevKit | uses the built-in TWAI (CAN) peripheral |
| SN65HVD230 3.3 V CAN transceiver | leave the 120 Ω terminator jumper **open** |
| 5 V relay module (KY-019 style) | active-HIGH; COM + NO used as dry contact |
| 12 V → 5 V 3 A buck converter | set to 5.00 V |
| 2 A blade fuse + holder | inline on the 12 V feed |

### Wiring

```
SN65HVD230            ESP32
  3.3V   ────────────  3V3
  GND    ────────────  GND
  CTX    ────────────  GPIO5  (CAN-TX)
  CRX    ────────────  GPIO4  (CAN-RX)
  CANH   ────────────  K-CAN High  (twisted pair)
  CANL   ────────────  K-CAN Low   (twisted pair)

Relay module          ESP32
  +/VCC  ────────────  5V
  –/GND  ────────────  GND
  S/IN   ────────────  GPIO16

Relay COM/NO          across the two target car wires
```

Power: switched 12 V → 2 A fuse → buck → 5 V → ESP32 and relay VCC. All grounds
common. Never apply 5 V to the SN65HVD230 — it is a 3.3 V part.

## Firmware

[`sportbutton.ino`](sportbutton.ino) is an Arduino sketch. It needs the ESP32
board package; the TWAI driver ships with the ESP32 Arduino core, so there are no
extra libraries.

1. Arduino IDE → Board Manager → add
   `https://dl.espressif.com/dl/package_esp32_index.json`
2. Board: **ESP32 Dev Module**, upload speed 921600
3. Open `sportbutton.ino`, upload, open the serial monitor at 115200 baud

Or with `arduino-cli`:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 .
arduino-cli upload  --fqbn esp32:esp32:esp32 -p /dev/cu.usbserial-XXXX .
```

### Serial commands

| Key | Action |
|-----|--------|
| `t` | run the on-device test suite (15 cases) |
| `s` | show detailed statistics |
| `r` | reset statistics |
| `d` | toggle debug output for every `0x1D6` frame |
| `h` | help |

The on-device suite runs the same frame table as the desktop suite against the
live detection path, including the relay timing.

## Desktop tests

Detection logic lives in [`can_messages.h`](can_messages.h) as a pure inline
function, so it can be validated on a laptop before flashing:

```bash
make test        # builds and runs all 15 cases
make protocol    # 5-case protocol walkthrough
make validate    # clean + tests + arduino-cli compile check
```

Expected output: 15/15 passed, 0 false positives, 0 false negatives.

## Repository layout

```
sportbutton.ino     ESP32 firmware (TWAI listener + relay pulse + self-test)
can_messages.h      CAN IDs, pin/timing constants, detection function
can_test_data.h     15-frame validation table incl. negative/edge cases
test_runner.cpp     desktop suite over can_test_data.h
protocol_test.cpp   walkthrough of the real-world button cases
can_analysis.md     findings from the K-CAN capture
hardware_design.md  BOM, wiring, power, troubleshooting
relay_test/         minimal sketch to prove the relay clicks
trace2.candump      raw K-CAN capture used for the analysis
```

## Safety

This project switches a car circuit and taps a vehicle CAN bus. Mistakes can
damage the car or create a safety hazard.

- Disconnect the battery negative before wiring anything.
- Fuse the 12 V feed (2 A) and keep the relay contacts dry — no voltage on them.
- The relay is only a dry contact; it does not isolate or protect the load.
- Keep CANH/CANL twisted and short; leave the transceiver's 120 Ω terminator open.
- Bench-test everything (CAN reception, relay click, 5.00 V rail) before the car.
- Anything that changes vehicle behaviour may be illegal or unsafe on public
  roads. This is a hobby/bench project, provided with no warranty — you are
  responsible for how you use it.

## License

MIT — see [LICENSE](LICENSE).
