# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

This is an **ESPHome external component** that integrates Zehnder ComfoAir 350/400 ventilation units with Home Assistant over RS232/UART (9600 baud, 8N1). The ESP device acts as a bridge, periodically polling the ComfoAir unit and exposing sensors, climate control, and binary sensors to Home Assistant.

## Linting

The only CI command is pylint on all Python files:

```bash
pip install pylint
pylint $(git ls-files '*.py')
```

Python versions supported: 3.8, 3.9, 3.10.

There is no build step or test suite — the component is validated by ESPHome's own build system when used in a real ESPHome config. The dev container (`.devcontainer/`) clones ESPHome locally for IntelliSense and manual testing.

## Architecture

### Python (`components/comfoair/__init__.py`)

The Python layer is ESPHome's codegen layer. It runs at **compile time** on the host machine, not on the ESP device. Two key functions:

- `CONFIG_SCHEMA` — validates the user's YAML config. Extends `climate.climate_schema()` and adds optional sensor/binary_sensor entries (23 total). All sensors are optional.
- `to_code()` — generates C++ instantiation and wiring code. For each configured sensor, it calls the corresponding C++ setter (e.g. `set_fan_supply_air_percentage()`).

### C++ (`components/comfoair/comfoair.h`)

`ComfoAirComponent` inherits from:
- `climate::Climate` — exposes fan mode + temperature setpoint to Home Assistant
- `PollingComponent` (600 ms) — drives the polling cycle
- `uart::UARTDevice` — serial I/O
- `api::CustomAPIDevice` — registers custom HA services for speed overrides

**Polling cycle** (`update()`, 7 steps, cycling via `update_counter_`):
1. Fan status (speeds %, RPM)
2. Valve status (bypass, preheating)
3. Sensor data (enthalpy temperature)
4. Ventilation level (current mode + stored speed levels)
5. Temperatures (4 sensors: outside, supply, return, exhaust)
6. Error/filter status
7. Bypass control (bypass factor, summer mode)

**Message reception** (`loop()`) reads bytes one at a time, assembles frames, validates the checksum, then calls `parse_data_()` which publishes values to all non-null sensor pointers.

### Protocol (`components/comfoair/messages.h`)

Frame format:
```
0x07 0xF0 0x00 <ID> <LEN> <DATA...> <CHECKSUM> 0x07 0x0F
```

- Checksum = `sum_of_all_frame_bytes + 0xAD`
- ACK = `0x07 0xF3` (2 bytes, no data)
- `0x07` bytes inside data are escaped as `0x07 0x07`
- Response ID = request ID + 1 (e.g. `GET_FAN_STATUS` = `0x0B`, response = `0x0C`)

Temperature encoding used throughout: `temp_C = (raw_byte / 2.0) - 20.0`

Fan RPM: `RPM = 1875000 / timer_value` (16-bit counter from device)

### Climate control mapping

| HA fan mode | ComfoAir level |
|-------------|---------------|
| `AUTO`      | 0             |
| `OFF`       | 1             |
| `LOW`       | 2             |
| `MEDIUM`    | 3             |
| `HIGH`      | 4             |

Only `FAN_ONLY` climate mode is supported. Temperature setpoint range: 12–29°C.

Custom HA services registered in `setup()`:
- `climate_set_speeds` — set one direction (exhaust or supply) at all 4 levels
- `climate_set_all_speeds` — set all 8 speed values (supply + exhaust × 4 levels)
- `climate_set_current_mode_speeds` — set exhaust/supply % for the active level only

Speed levels are persisted in `ventilation_levels_[8]` (read from the device, updated on set commands).
