# Home Assistant & Lovelace Integration

The ComfoAir ESPHome component is specifically tuned to work with the **Lovelace ComfoAir card** by @wichers.

## Entity ID Mapping
To ensure the card functions correctly, the following entity IDs are required in Home Assistant:

| ESPHome Key | HA Entity ID (if device name is `comfoair`) | Card Usage |
| ----------- | ------------------------------------------- | ---------- |
| `outside_air_temperature` | `sensor.comfoair_outside_air_temperature` | Intake Temp |
| `exhaust_air_temperature` | `sensor.comfoair_exhaust_air_temperature` | Exhaust Temp |
| `return_air_temperature` | `sensor.comfoair_return_air_temperature` | Room Out Temp |
| `supply_air_temperature` | `sensor.comfoair_supply_air_temperature` | Room In Temp |
| `fan_speed_supply` | `sensor.comfoair_intake_fan_speed_rpm` | Fan 1 RPM |
| `fan_speed_exhaust` | `sensor.comfoair_exhaust_fan_speed_rpm` | Fan 2 RPM |
| `is_filter_full` + `text_sensor` | `sensor.comfoair_filter_status` | Filter Warning ("Full") |
| `is_bypass_valve_open` | `binary_sensor.comfoair_bypass_valve_open` | Bypass Icon |
| `is_preheating` | `binary_sensor.comfoair_preheating_state` | Pre-heat Icon |
| `is_summer_mode` | `binary_sensor.comfoair_summer_mode` | Summer Icon |

## Troubleshooting Template Errors
If a template error occurs in Home Assistant like `UndefinedError: 'binary_sensor' is undefined`, always use the `states()` function:

- **Correct**: `{{ is_state('binary_sensor.comfoair_summer_mode', 'off') }}`
- **Incorrect**: `{{ binary_sensor.comfoair_summer_mode }}`

## Dashboard YAML Snippet
For the [Simple Thermostat Card](https://github.com/nervetattoo/simple-thermostat):

```yaml
type: custom:simple-thermostat
entity: climate.comfoair
layout:
  step: 1
control:
  fan:
    low: {icon: mdi:fan-speed-1}
    medium: {icon: mdi:fan-speed-2}
    high: {icon: mdi:fan-speed-3}
```
