---
name: ComfoAir ESPHome
description: Knowledge and implementation details for Zehnder ComfoAir protocol integration with ESPHome.
---

# ComfoAir ESPHome Skill

This skill provides deep technical knowledge about the Zehnder ComfoAir (SAE Gen 1) protocol and its implementation as an ESPHome component.

## Core Protocol Knowledge
The ComfoAir protocol uses an RS232 interface at 9600 bps, 8 data bits, no parity, 1 stop bit.

- **Start of Frame**: `0x07 0xF0`
- **End of Frame**: `0x07 0x0F`
- **Checksum**: Sum of all bytes (excluding head/tail) + `0xAD`, modulo 256.
- **Byte Stuffing**: If `0x07` appears in the data, it must be escaped as `0x07 0x07`.

## Key Commands
- `0x00 0xCD`: Get Ventilation Status (Fans, bypass, status).
- `0x00 0xD1`: Get Temperatures.
- `0x00 0xD3`: Set Comfort Temperature.
- `0x00 0x99`: Set Ventilation Level (0=Auto, 1=Low, 2=Medium, 3=High, 4=Very High).
- `0x00 0xDD`: Get Operating Hours.

## Implementation Patterns

### Auto Temperature Balance
An intelligent thermal management layer that adjusts fan levels based on internal/external delta:
1. Only active in `AUTO` fan mode.
2. Monitors unit's internal bypass decision.
3. If bypass is helping (Cooling with outside air/Heating with warmer outside air):
   - Boosts speed to Level 3 (`Medium`).
   - If Delta > 1.5C, boosts to Level 4 (`High`).
4. Prevents unnecessary wear by only running every 5 minutes.

### Lovelace Card Compatibility
Compatibility with `lovelace-comfoair` requires specific entity IDs:
- Temperatures: `sensor.comfoair_<site>_air_temperature`
- Fans: `sensor.comfoair_intake_fan_speed_rpm`, `sensor.comfoair_exhaust_fan_speed_rpm`
- Filter: A text sensor `sensor.comfoair_filter_status` returning "Full" or "Ok".

## Usage in Coding Tasks
When assisting with ComfoAir, always verify the checksum logic using the provided C++ tests and ensure that any new sensors follow the naming conventions established for dashboard compatibility.

See `resources/` for detailed protocol mappings.
