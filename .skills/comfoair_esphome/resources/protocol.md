# Zehnder ComfoAir RS232 Protocol Specs (Gen 1)

This specification covers the protocol used by ComfoAir 350, 450, 550 and similar units with RS232 (TTL 5V or RS232-C).

## UART Settings
- Baud Rate: 9600
- Data Bits: 8
- Parity: None
- Stop Bits: 1

## Message Framing
Each command or response frame follows this structure:

| Part | Bytes | Description |
| ---- | ----- | ----------- |
| Start | `0x07 0xF0` | Head (Start of Communication) |
| ID | `0x00 0xXX` | Command/Response Identifier |
| Length | `0xXX` | Length of data bytes only |
| Data | `[...]` | Zero or more data bytes |
| Checksum | `0xXX` | Calculated as `Sum(ID + Length + Data) + 0xAD` |
| End | `0x07 0x0F` | Tail (End of Communication) |

### Important Rules
- **Byte Stuffing**: If any byte in ID, Length, Data, or Checksum is `0x07`, it must be immediately followed by another `0x07` (escaped).
- **Checksum Calculation**:
  ```cpp
  uint8_t sum = 0;
  for each byte in (ID, Length, Data): sum += byte;
  uint8_t checksum = sum + 0xAD;
  ```

## Key Commands and Data Structures

### 0x0C: COMFOAIR_GET_FAN_STATUS_RESPONSE (6 bytes)
- `msg[0]`: Supply Fan %
- `msg[1]`: Exhaust Fan %
- `msg[2-3]`: Supply Fan Raw (Formula: `1875000.0f / raw`)
- `msg[4-5]`: Exhaust Fan Raw (Formula: `1875000.0f / raw`)

### 0xCD: COMFOAIR_GET_VENTILATION_LEVEL_RESPONSE (12 bytes)
- `msg[0]`: Level (0x01-0x04)
- `msg[6]`: Return air level %
- `msg[7]`: Supply air level %
- `msg[8]`: Current level (used for climate mode)

### 0xD1: COMFOAIR_GET_TEMPERATURES_RESPONSE (9 bytes)
- `msg[0-4]`: Fresh in, Supply, Return, Exhaust, Enthalpy (formula: `(byte / 2.0f) - 20.0f`)

### 0xDD: COMFOAIR_GET_OPERATING_HOURS_RESPONSE (20 bytes)
- `msg[0-2]`: Supply Fan Hours (3-byte Big Endian)
- `msg[3-5]`: Exhaust Fan Hours
- `msg[6-8]`: Bypass Hours
- `msg[9-11]`: Pre-heater Hours
- `msg[12-14]`: Filter Hours
- `msg[15-17]`: EWT Hours
- `msg[18-19]`: Pre-heater Status
