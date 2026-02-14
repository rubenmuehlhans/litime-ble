# ESPHome LiTime BMS BLE Component

[![Build](https://github.com/rubenmuehlhans/litime-ble/actions/workflows/build.yml/badge.svg)](https://github.com/rubenmuehlhans/litime-ble/actions/workflows/build.yml)

ESPHome external component to monitor and control LiTime LiFePO4 batteries via Bluetooth Low Energy (BLE).

## Supported Devices

| Manufacturer | Models | Tested |
|---|---|---|
| **LiTime** | 12V, 24V, 48V LiFePO4 with BLE5 | Yes |
| **Redodo** | 12V, 24V LiFePO4 with BLE | Expected |
| **PowerQueen** | 12V, 24V LiFePO4 with BLE | Expected |

All devices using the same BLE protocol (service `0xFFE0`, name prefix `LT-`) should be compatible.

## Features

- **Auto-Discovery** -- built-in BLE scanner finds all LiTime batteries in range and logs their MAC addresses
- **22 Sensors** -- voltage, current, power, SOC, SOH, individual cell voltages, temperatures, capacity, cycle count
- **4 Binary Sensors** -- charging, discharging, balancing, online status
- **2 Text Sensors** -- decoded protection status and error flags
- **2 Switches** -- enable/disable charge and discharge MOSFETs
- **Multi-Battery** -- monitor multiple batteries simultaneously from a single ESP32
- **Offline Detection** -- sensors automatically report `NaN` after 5 missed responses

## Requirements

- ESP32 board (BLE required -- ESP8266 is **not** supported)
- ESPHome 2024.2.0 or later
- Framework: `esp-idf` (recommended) or `arduino`

## Quick Start

### Step 1: Find Your Battery's MAC Address

Flash this minimal configuration to your ESP32. It will scan for LiTime batteries and log their MAC addresses:

```yaml
esphome:
  name: litime-scanner

esp32:
  board: esp32dev
  framework:
    type: esp-idf

logger:
  level: DEBUG

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

external_components:
  - source: github://rubenmuehlhans/litime-ble@main

esp32_ble_tracker:
  scan_parameters:
    active: true

litime_bms_ble:
  - id: scanner
```

> **Note:** `active: true` is important -- some batteries only send their full name in the scan response, which requires an active scan.

Check the ESPHome log output for entries like:

```
[I][litime_bms_ble]: ========================================
[I][litime_bms_ble]:   Found compatible BLE device!
[I][litime_bms_ble]:   Name:  "L-12345"
[I][litime_bms_ble]:   MAC:   AA:BB:CC:DD:EE:FF
[I][litime_bms_ble]:   RSSI:  -67 dBm
[I][litime_bms_ble]:   Match: name prefix "L-"
[I][litime_bms_ble]:   Add to your YAML:
[I][litime_bms_ble]:     ble_client:
[I][litime_bms_ble]:       - mac_address: "AA:BB:CC:DD:EE:FF"
[I][litime_bms_ble]: ========================================
```

### Step 2: Configure Monitoring

Replace the scanner config with the full monitoring configuration using the discovered MAC address:

```yaml
esphome:
  name: litime-bms

esp32:
  board: esp32dev
  framework:
    type: esp-idf

logger:
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

api:
  encryption:
    key: !secret api_encryption_key

ota:
  - platform: esphome

external_components:
  - source: github://rubenmuehlhans/litime-ble@main

esp32_ble_tracker:
  scan_parameters:
    active: true

ble_client:
  - mac_address: "AA:BB:CC:DD:EE:FF"  # <-- your MAC here
    id: bms_client

litime_bms_ble:
  - id: bms1
    ble_client_id: bms_client
    update_interval: 5s

sensor:
  - platform: litime_bms_ble
    litime_bms_ble_id: bms1
    total_voltage:
      name: "Battery Voltage"
    current:
      name: "Battery Current"
    power:
      name: "Battery Power"
    state_of_charge:
      name: "Battery SOC"

binary_sensor:
  - platform: litime_bms_ble
    litime_bms_ble_id: bms1
    online:
      name: "Battery Online"
```

## Configuration Reference

### Hub Configuration

```yaml
litime_bms_ble:
  - id: bms1
    ble_client_id: bms_client   # Required for monitor mode
    update_interval: 5s          # Optional, default: 5s
```

| Option | Type | Default | Description |
|---|---|---|---|
| `id` | ID | *required* | Unique identifier for this instance |
| `ble_client_id` | ID | *omit for scanner* | Reference to a `ble_client` entry. Omit this to enable scanner mode. |
| `update_interval` | time | `5s` | How often to poll the BMS |

### Sensors

```yaml
sensor:
  - platform: litime_bms_ble
    litime_bms_ble_id: bms1
```

All sensors are optional. Only configure the ones you need.

| Sensor | Unit | Description |
|---|---|---|
| `total_voltage` | V | Total battery voltage |
| `current` | A | Current (positive = charging, negative = discharging) |
| `power` | W | Calculated: voltage x current |
| `state_of_charge` | % | State of charge (SOC) |
| `state_of_health` | % | State of health (SOH) |
| `cell_temperature` | °C | Cell temperature |
| `mosfet_temperature` | °C | MOSFET temperature |
| `remaining_capacity` | Ah | Remaining capacity |
| `full_charge_capacity` | Ah | Full charge capacity |
| `discharge_cycles` | -- | Total charge/discharge cycle count |
| `total_discharge_ah` | Ah | Cumulative discharge |
| `min_cell_voltage` | V | Lowest cell voltage (calculated) |
| `max_cell_voltage` | V | Highest cell voltage (calculated) |
| `delta_cell_voltage` | V | Cell voltage difference: max - min |
| `cell_voltage_1` ... `cell_voltage_16` | V | Individual cell voltages |

**Cell count by battery type:**

| Battery | Cells |
|---|---|
| 12V (4S) | `cell_voltage_1` to `cell_voltage_4` |
| 24V (8S) | `cell_voltage_1` to `cell_voltage_8` |
| 48V (16S) | `cell_voltage_1` to `cell_voltage_16` |

Cells that are not present in your battery will simply not report values.

### Binary Sensors

```yaml
binary_sensor:
  - platform: litime_bms_ble
    litime_bms_ble_id: bms1
```

| Sensor | Description |
|---|---|
| `charging` | Battery is currently being charged |
| `discharging` | Battery is currently discharging |
| `balancing` | Cell balancing is active |
| `online` | BLE connection is active |

### Text Sensors

```yaml
text_sensor:
  - platform: litime_bms_ble
    litime_bms_ble_id: bms1
```

| Sensor | Description |
|---|---|
| `protection_status` | Active protection flags as human-readable text (e.g. "Overcharge, High temp 1") or "OK" |
| `failure_status` | Active failure flags or "OK" |

**Decoded protection flags:**

- Overcharge
- Over-discharge
- Charge overcurrent
- Discharge overcurrent
- High temp 1 / High temp 2
- Low temp 1 / Low temp 2
- Short circuit

### Switches

```yaml
switch:
  - platform: litime_bms_ble
    litime_bms_ble_id: bms1
```

| Switch | Description |
|---|---|
| `charging_switch` | Enable/disable the charge MOSFET |
| `discharging_switch` | Enable/disable the discharge MOSFET |

**Warning:** Disabling the discharge MOSFET will cut off all loads connected to the battery. Use with caution.

## Multiple Batteries

You can monitor multiple batteries from a single ESP32. The ESP32 supports up to 3 simultaneous BLE connections (depending on available RAM):

```yaml
ble_client:
  - mac_address: "AA:BB:CC:DD:EE:FF"
    id: bms_client_1
  - mac_address: "11:22:33:44:55:66"
    id: bms_client_2

litime_bms_ble:
  - id: bms1
    ble_client_id: bms_client_1
  - id: bms2
    ble_client_id: bms_client_2

sensor:
  - platform: litime_bms_ble
    litime_bms_ble_id: bms1
    total_voltage:
      name: "Battery 1 Voltage"
    state_of_charge:
      name: "Battery 1 SOC"

  - platform: litime_bms_ble
    litime_bms_ble_id: bms2
    total_voltage:
      name: "Battery 2 Voltage"
    state_of_charge:
      name: "Battery 2 SOC"
```

## Scanner + Monitor Combined

You can run the scanner alongside active battery connections. This is useful if you add new batteries later:

```yaml
litime_bms_ble:
  - id: scanner
  - id: bms1
    ble_client_id: bms_client_1
```

The scanner listens passively to BLE advertisements and does not consume a BLE connection slot.

## Known Limitations

- **One BLE client at a time:** The battery only accepts one active BLE connection. While the ESP32 is connected, the LiTime smartphone app cannot connect (and vice versa).
- **ESP32 only:** This component requires an ESP32. ESP8266 and RP2040 boards do not have BLE client support.
- **BLE range:** Typical range is 5-10 meters. Walls and metal enclosures reduce range.
- **First connection:** After flashing, the first BLE connection may take 10-30 seconds to establish.

## BLE Protocol

The component communicates with the battery's BMS over BLE using a proprietary protocol:

| Property | Value |
|---|---|
| Service UUID | `0000FFE0-0000-1000-8000-00805F9B34FB` |
| Notify Characteristic (RX) | `0000FFE1-...` |
| Write Characteristic (TX) | `0000FFE2-...` |
| Advertising Name Prefix | `LT-` |

**Command frame format** (8 bytes):

```
Byte:  [0]   [1]   [2]   [3]   [4]    [5]   [6]   [7]
       0x00  0x00  0x04  0x01  CMD    0x55  0xAA  CHECKSUM
```

Checksum = `byte[2] + byte[3] + byte[4]`

**Status response** (104 bytes, little-endian):

| Offset | Length | Type | Description |
|---|---|---|---|
| 8-11 | 4 | uint32 | Total voltage (mV) |
| 16-47 | 32 | 16x uint16 | Cell voltages (mV) |
| 48-51 | 4 | int32 | Current (mA, positive = charging) |
| 52-53 | 2 | int16 | Cell temperature (°C) |
| 54-55 | 2 | int16 | MOSFET temperature (°C) |
| 62-63 | 2 | uint16 | Remaining capacity (x0.01 Ah) |
| 64-65 | 2 | uint16 | Full charge capacity (x0.01 Ah) |
| 76-79 | 4 | uint32 | Protection flags (bitmask) |
| 80-83 | 4 | uint32 | Failure flags (bitmask) |
| 84-87 | 4 | uint32 | Balancing state |
| 88-89 | 2 | uint16 | Battery state (0=discharging, 1=charging) |
| 90-91 | 2 | uint16 | SOC (%) |
| 92-95 | 4 | uint32 | SOH (%) |
| 96-99 | 4 | uint32 | Discharge cycle count |
| 100-103 | 4 | uint32 | Total discharge (mAh) |

## Troubleshooting

### Battery not found by scanner

- Make sure `active: true` is set in `esp32_ble_tracker` -- some batteries only advertise their name in the scan response
- Make sure the battery is powered on and BLE is enabled
- Check that no other device (phone app) is currently connected to the battery -- a BLE peripheral typically only accepts one connection
- Reduce distance between ESP32 and battery (BLE range: ~5-10m)
- Set logger level to `DEBUG` for more details

The scanner detects devices using these criteria (in order):

| Match | Pattern | Example |
|---|---|---|
| Name prefix | `LT-` | `LT-B12345` |
| Name prefix | `L-` | `L-12345` |
| Name contains | `litime` (case-insensitive) | `LiTime_BMS_01` |
| Name contains | `redodo` (case-insensitive) | `Redodo-100Ah` |
| Name contains | `powerqueen` (case-insensitive) | `PowerQueen_01` |
| Service UUID | `0xFFE0` in advertisement | *(any name or no name)* |

If your battery uses a different name pattern, you can find its MAC address using the **nRF Connect** app on your phone and configure it manually in the `ble_client` section.

### Connection drops frequently

- Increase `update_interval` to reduce BLE traffic (e.g. `10s` or `15s`)
- Reduce the number of simultaneous BLE connections
- Ensure stable power supply to the ESP32

### Sensors show NaN

- The component marks all sensors as `NaN` after 5 consecutive missed responses
- Check if the BLE connection is still active (`online` binary sensor)
- Verify the MAC address is correct

### Build fails

- Ensure you're using `esp-idf` framework (recommended) or `arduino`
- This component requires ESPHome 2024.2.0 or later
- ESP32 is required (not ESP8266)

## Project Structure

```
components/
  litime_bms_ble/
    __init__.py          # Hub component (scanner + monitor modes)
    sensor.py            # Sensor platform definitions
    binary_sensor.py     # Binary sensor platform definitions
    text_sensor.py       # Text sensor platform definitions
    switch.py            # Switch platform definitions
    litime_bms_ble.h     # C++ header
    litime_bms_ble.cpp   # C++ implementation
```

## License

MIT

## Acknowledgments

- Protocol reverse-engineering based on community efforts around LiTime/Redodo BLE batteries
- Inspired by [syssi's ESPHome BMS components](https://github.com/syssi) (JBD, JK, ANT)
