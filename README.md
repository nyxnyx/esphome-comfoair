# ComfoAir ESPHome Component

Port of Zehnder ComfoAir (CA350 / CA550 / WHR930) RS232 protocol to ESPHome.io firmware.

## Features
- **Climate Entity**: Controls operational fan stages (Absent, Low, Medium, High) and comfort temperature.
- **Full Telemetry**: Outside, supply, return, and exhaust temperatures, bypass state, preheating state, filter status, and RPM sensors.
- **Stage Percentage Sensors**: Read programmed fan speed percentages for all 4 stages (Exhaust & Supply) via command `0x00 0xCD` / `0xCE`.
- **Set Ventilation Levels**: Program custom speed percentages for Absent, Low, Medium, and High stages using command `0x00 0xCF` directly from ESPHome without needing physical CC-Ease installer menus (P20–P27).

## Configuration Example

```yaml
comfoair:
  id: my_comfoair
  name: "ComfoAir 350"
  uart_id: uart_bus

  # Telemetry
  outside_air_temperature:
    name: "Outside Temperature"
  supply_air_temperature:
    name: "Supply Temperature"
  return_air_temperature:
    name: "Return Temperature"
  exhaust_air_temperature:
    name: "Exhaust Temperature"

  # Live fan telemetry
  fan_supply_air_percentage:
    name: "Fan Supply %"
  fan_exhaust_air_percentage:
    name: "Fan Exhaust %"
  fan_speed_supply:
    name: "Supply Fan RPM"
  fan_speed_exhaust:
    name: "Exhaust Fan RPM"

  # Programmed stage percentages
  fan_exhaust_absent:
    name: "Exhaust Stage 0 (Absent) %"
  fan_exhaust_low:
    name: "Exhaust Stage 1 (Low) %"
  fan_exhaust_medium:
    name: "Exhaust Stage 2 (Medium) %"
  fan_exhaust_high:
    name: "Exhaust Stage 3 (High) %"
  fan_supply_absent:
    name: "Supply Stage 0 (Absent) %"
  fan_supply_low:
    name: "Supply Stage 1 (Low) %"
  fan_supply_medium:
    name: "Supply Stage 2 (Medium) %"
  fan_supply_high:
    name: "Supply Stage 3 (High) %"
```

### Setting Custom Stage Percentages

You can set custom fan speed percentages programmatically via C++ / lambda:

```yaml
button:
  - platform: template
    name: "Apply Ventilation Levels"
    on_press:
      then:
        - lambda: |-
            // set_ventilation_levels(ex_absent, ex_low, ex_med, ex_high, su_absent, su_low, su_med, su_high, current_speed)
            id(my_comfoair)->set_ventilation_levels(15, 30, 45, 70, 15, 30, 45, 70);
```

## Visualization & UI
- [simple-thermostat](https://github.com/nervetattoo/simple-thermostat)
- [lovelace-comfoair](https://github.com/wichers/lovelace-comfoair)