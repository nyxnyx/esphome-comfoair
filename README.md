# ComfoAir ESPHome Component

This project provides a robust integration for Zehnder ComfoAir (350, 450, 550) heat recovery ventilation (HRV) units into [ESPHome](https://esphome.io).

## Installation

Add the following to your ESPHome YAML configuration:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/nyxnyx/esphome-comfoair
    components: [comfoair]

uart:
  id: uart_bus
  baud_rate: 9600
  tx_pin: TX  # Adjust for your board
  rx_pin: RX  # Adjust for your board

comfoair:
  id: my_comfoair
  name: "ComfoAir"
  uart_id: uart_bus
  # Add sensors/switches from the list below:
```

## Available Features

Below is a complete list of all supported properties you can add under the `comfoair:` component.

### Core Fan Sensors
| Key | Type | Description | Unit |
|-----|------|-------------|------|
| `fan_supply_percentage` | Sensor | Current supply fan power (%) | % |
| `fan_exhaust_percentage` | Sensor | Current exhaust fan power (%) | % |
| `fan_speed_supply` | Sensor | Rotational speed of supply fan | rpm |
| `fan_speed_exhaust` | Sensor | Rotational speed of exhaust fan | rpm |

### Temperature Sensors
| Key | Type | Description | Unit |
|-----|------|-------------|------|
| `outside_air_temperature` | Sensor | Fresh air entering from outside (T1) | °C |
| `supply_air_temperature` | Sensor | Fresh air entering the house (T2) | °C |
| `return_air_temperature` | Sensor | Warm air leaving the rooms (T3) | °C |
| `exhaust_air_temperature` | Sensor | Used air leaving to outside (T4) | °C |
| `enthalpy_temperature` | Sensor | Enthalpy sensor value (if present) | °C |
| `ewt_temperature` | Sensor | Ground heat exchanger value (if present) | °C |

### Status & Binary Sensors
| Key | Type | Description |
|-----|------|-------------|
| `is_bypass_valve_open` | Binary | State of the heat recovery bypass |
| `is_preheating` | Binary | Internal pre-heater status |
| `is_summer_mode` | Binary | Unit is in summer bypass mode |
| `is_supply_fan_active` | Binary | Status of the supply fan |
| `is_filter_full` | Binary | Indicates if filter maintenance is required |

### Operating Hours (Counters)
| Key | Unit | Description |
|-----|------|-------------|
| `fan_hours_supply` | h | Total runtime of supply fan |
| `fan_hours_exhaust` | h | Total runtime of exhaust fan |
| `bypass_hours` | h | Total hours bypass was open |
| `preheater_hours` | h | Total hours pre-heater was active |
| `filter_hours` | h | Hours since last filter change |
| `ewt_hours` | h | Ground heat exchanger runtime |

### Automation & Control
| Key | Type | Description |
|-----|------|-------------|
| `auto_balance` | Switch | **Intelligent Speed Boost**: Automatically adjusts fan speed (0-3) based on thermal efficiency. See details below. |
| `error_code` | Sensor | Last reported error code for diagnostics. |

## Feature: Auto Temperature Balance

The `auto_balance` feature (Thermal Power Guidance) intelligently adjusts the fan speed to reach your comfort temperature faster while minimizing energy loss.

### Requirements
To use this feature, you must have the following sensors enabled in your `comfoair:` configuration:
- `supply_air_temperature`
- `return_air_temperature`
- `auto_balance` (the switch)
- `is_preheating` (optional, but recommended for energy saving)

### How it works
1. **Activate the Switch**: Turn on the "Auto Temperature Balance" switch in Home Assistant.
2. **Set Target Temperature**: Choose your desired comfort temperature (e.g., 21°C) in your Climate card.

**Automation Logic**:
- **Helping Mode**: If the unit provides air that moves the house towards the target (e.g., 18°C supply when you want 21°C cooling), it will **boost the fans** (up to Stage 3).
- **Protective Mode**: If the outside air is counter-productive (e.g., 30°C supply when you want 21°C cooling), it will **reduce speed to Stage 0 (Absent)** to preserve the indoor climate.
- **Safety**: If `is_preheating` is active, the system throttles speed to avoid excessive heating costs.

---

## Service Functions

### Reset Filter Timer
You can reset the filter maintenance timer directly from ESPHome using a template button:

```yaml
button:
  - platform: template
    name: "Reset Filter Timer"
    on_press:
      then:
        - lambda: |-
            id(my_comfoair)->reset_filter();
```

## Visualization
For a premium look in Lovelace, we recommend using the [Simple Thermostat Card](https://github.com/nervetattoo/simple-thermostat) or [lovelace-comfoair](https://github.com/wichers/lovelace-comfoair).

---
*Maintained by @nyxnyx. Forked from @wichers.*
