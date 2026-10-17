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

### 0xD9 / 0xDA: COMFOAIR_GET_ERROR_STATE_RESPONSE (17 bytes)
- `msg[0]`: Current raw error code (E-00 to E-99)
- `msg[5]`: Temperature sensor faults (bit 0=T1, bit 1=T2, bit 2=T3, bit 3=T4, bit 4=Preheater)
- `msg[6]`: Motor and damper faults (bit 0=Supply Fan, bit 1=Exhaust Fan, bit 2=Bypass)
- `msg[8]`: Filter replacement flag (0=OK, 1=Dirty)

### 0xC9 / 0xCA: COMFOAIR_GET_DELAYS_RESPONSE & 0xCB SET (8 bytes)
- `msg[0]`: Bathroom switch-on delay (P11 in minutes)
- `msg[1]`: Bathroom switch-off overrun delay (P12 in minutes)
- `msg[2]`: Ventilation boost delay (P17 in minutes)
- `msg[3]`: Filter timer weeks (P13 in weeks)

### 0x13 / 0x14: COMFOAIR_GET_ANALOG_INPUTS_RESPONSE (4 bytes)
- `msg[0-3]`: Raw analog 0-10V inputs 1, 2, 3, 4 (voltage / percentage)

### 0xEB / 0xEC: COMFOAIR_GET_EWT_REHEATER_RESPONSE & 0xED SET (7 bytes)
- `msg[0]`: EWT damper state (0=closed, 1=open)
- `msg[1]`: EWT lower activation threshold
- `msg[2]`: EWT upper activation threshold
- `msg[3]`: Secondary reheater state
- `msg[4]`: Reheater target temperature

### 0xD5 / 0xD6: COMFOAIR_GET_MAIN_STATUS_RESPONSE & 0xD7 SET (8 bytes)
- `msg[0]`: Damper flags (bit 1=force close, bit 2=force open, bit 3=party mode, bit 4=chimney mode)
- `msg[1]`: Party mode remaining timer (minutes)
- `msg[2]`: Chimney mode remaining timer (minutes)
