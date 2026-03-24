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

### Auto Temperature Balance (Thermal Power Guidance)
A sophisticated thermal management layer that adjusts fan levels (0-3) based on real-time heat/cool exchange:
1. **Core Logic**: Uses `Supply Air Temp` vs `Return Air Temp` to calculate positive/negative **Thermal Power**.
2. **Heuristic**: Matches Demand (Target vs Room) against current Power.
   - **Helping**: If supplying cooler air while cooling is needed (or warmer air while heating is needed), it ramps up fan speed (Level 2-4) proportionally to the demand.
   - **Counter-productive**: If the air exchange is moving the house temp in the wrong direction, it throttles to **Level 1 (Absent)** to minimize loss.
3. **Optimizations**: 
   - Throttles boost if `is_preheating` is active to save energy.
   - Includes a 5-minute debounce timer but allows immediate forced recalculation on switch toggles.

### Technical Implementation Notes
- **Component Lifecycle**: Custom switches must inherit from BOTH `Switch` and `Component` and be registered via `register_component` to ensure state persistence and proper synchronization with Home Assistant.
- **Access Control**: Forward-declared components must use `friend class` declarations to allow switches to call protected logic methods like `run_auto_balance_`.
- **Circular Dependencies**: Implement method bodies (like `write_state`) using `inline` at the end of the header file, after all related classes are fully defined.

## Usage in Coding Tasks
When assisting with ComfoAir, always verify the checksum logic using the provided C++ tests and ensure that any new sensors follow the naming conventions established for dashboard compatibility.

See `resources/` for detailed protocol mappings and balancing formulas.
