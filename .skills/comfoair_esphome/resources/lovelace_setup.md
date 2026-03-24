# Home Assistant & Lovelace Integration

The ComfoAir ESPHome component is specifically tuned to work with the **Lovelace ComfoAir card** by @wichers.

## Entity ID Mapping
To ensure the card functions correctly, the following entity IDs are required in Home Assistant:

| ESPHome Key | HA Entity ID (Official GitHub) | HA Entity ID (HACS / szosti.eu) | Card Usage |
| ----------- | ----------------------------- | ------------------------------- | ---------- |
| `outside_air_temperature` | `sensor.comfoair_outside_air_temperature` | `sensor.comfoair_outside_temperature` | Intake Temp |
| `exhaust_air_temperature` | `sensor.comfoair_exhaust_air_temperature` | `sensor.comfoair_exhaust_temperature` | Exhaust Temp |
| `return_air_temperature` | `sensor.comfoair_return_air_temperature` | `sensor.comfoair_return_temperature` | Room Out Temp |
| `supply_air_temperature` | `sensor.comfoair_supply_air_temperature` | `sensor.comfoair_supply_temperature` | Room In Temp |
| `fan_speed_supply` | `sensor.comfoair_intake_fan_speed_rpm` | `sensor.comfoair_supply_fan_speed` | Fan 1 RPM |
| `fan_speed_exhaust` | `sensor.comfoair_exhaust_fan_speed_rpm` | `sensor.comfoair_exhaust_fan_speed` | Fan 2 RPM |
| `is_bypass_valve_open` | `binary_sensor.comfoair_bypass_valve_open` | `binary_sensor.comfoair_is_bypas_open` | Bypass Icon |
| `is_preheating` | `binary_sensor.comfoair_preheating_state` | `binary_sensor.comfoair_is_preheating_active` | Pre-heat Icon |
| `is_summer_mode` | `binary_sensor.comfoair_summer_mode` | `binary_sensor.comfoair_is_summer_mode` | Summer Icon |

> [!WARNING]
> **Typo Requirement**: The HACS / szosti.eu version of the card (JS line 80) expects the spelling `bypas` (with one 's') for the bypass entity ID: `binary_sensor.comfoair_is_bypas_open`. Do not "correct" this spelling in YAML if using this card.

> [!NOTE]
> **Fan Speeds**: The szosti.eu card uses `supply_fan_speed` instead of `intake_fan_speed_rpm`. Always confirm the card source URL before naming sensors.

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
