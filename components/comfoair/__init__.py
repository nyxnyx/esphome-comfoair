""" Creates module ComfoAir """
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import *
from esphome.components import uart
from esphome.components import climate
from esphome.components import sensor
from esphome.components import binary_sensor
from esphome.components import switch
from esphome import pins
comfoair_ns = cg.esphome_ns.namespace('comfoair')
ComfoAirComponent = comfoair_ns.class_('ComfoAirComponent', climate.Climate, cg.PollingComponent, uart.UARTDevice)
ComfoAirAutoBalanceSwitch = comfoair_ns.class_('ComfoAirAutoBalanceSwitch', switch.Switch, cg.Component)

DEPENDENCIES=['uart']
AUTO_LOAD = ['sensor', 'climate', 'binary_sensor', 'switch']
CONF_HUB_ID = 'comfoair'

CONF_FAN_SUPPLY_PERCENTAGE = "fan_supply_percentage"
CONF_FAN_EXHAUST_PERCENTAGE = "fan_exhaust_percentage"
CONF_FAN_SPEED_SUPPLY = "fan_speed_supply"
CONF_FAN_SPEED_EXHAUST = "fan_speed_exhaust"
CONF_IS_BYPASS_VALVE_OPEN = "is_bypass_valve_open"
CONF_IS_PREHEATING = "is_preheating"
CONF_OUTSIDE_AIR_TEMPERATURE = "outside_air_temperature"
CONF_SUPPLY_AIR_TEMPERATURE = "supply_air_temperature"
CONF_RETURN_AIR_TEMPERATURE = "return_air_temperature"
CONF_EXHAUST_AIR_TEMPERATURE = "exhaust_air_temperature"
CONF_ENTHALPY_TEMPERATURE = "enthalpy_temperature"
CONF_EWT_TEMPERATURE = "ewt_temperature"
CONF_REHEATING_TEMPERATURE = "reheating_temperature"
CONF_KITCHEN_HOOD_TEMPERATURE = "kitchen_hood_temperature"
CONF_RETURN_AIR_LEVEL = "return_air_level"
CONF_SUPPLY_AIR_LEVEL = "supply_air_level"
CONF_IS_SUPPLY_FAN_ACTIVE = "is_supply_fan_active"
CONF_IS_FILTER_FULL = "is_filter_full"
CONF_BYPASS_FACTOR = "bypass_factor"
CONF_BYPASS_STEP = "bypass_step"
CONF_BYPASS_CORRECTION = "bypass_correction"
CONF_IS_SUMMER_MODE = "is_summer_mode"
CONF_FAN_HOURS_SUPPLY = "fan_hours_supply"
CONF_FAN_HOURS_EXHAUST = "fan_hours_exhaust"
CONF_BYPASS_HOURS = "bypass_hours"
CONF_PREHEATER_HOURS = "preheater_hours"
CONF_FILTER_HOURS = "filter_hours"
CONF_EWT_HOURS = "ewt_hours"
CONF_ERROR_CODE = "error_code"
CONF_AUTO_BALANCE = "auto_balance"

helper_comfoair_list = [
    CONF_FAN_SUPPLY_PERCENTAGE,
    CONF_FAN_EXHAUST_PERCENTAGE,
    CONF_FAN_SPEED_SUPPLY,
    CONF_FAN_SPEED_EXHAUST,
    CONF_IS_BYPASS_VALVE_OPEN,
    CONF_IS_PREHEATING,
    CONF_OUTSIDE_AIR_TEMPERATURE,
    CONF_SUPPLY_AIR_TEMPERATURE,
    CONF_RETURN_AIR_TEMPERATURE,
    CONF_EXHAUST_AIR_TEMPERATURE,
    CONF_ENTHALPY_TEMPERATURE,
    CONF_EWT_TEMPERATURE,
    CONF_REHEATING_TEMPERATURE,
    CONF_KITCHEN_HOOD_TEMPERATURE,
    CONF_RETURN_AIR_LEVEL,
    CONF_SUPPLY_AIR_LEVEL,
    CONF_IS_SUPPLY_FAN_ACTIVE,
    CONF_IS_FILTER_FULL,
    CONF_BYPASS_FACTOR,
    CONF_BYPASS_STEP,
    CONF_BYPASS_CORRECTION,
    CONF_IS_SUMMER_MODE,
    CONF_FAN_HOURS_SUPPLY,
    CONF_FAN_HOURS_EXHAUST,
    CONF_BYPASS_HOURS,
    CONF_PREHEATER_HOURS,
    CONF_FILTER_HOURS,
    CONF_EWT_HOURS,
    CONF_ERROR_CODE,
]

comfoair_sensors_schemas = cv.Schema(
    {
cv.Optional(CONF_FAN_SUPPLY_PERCENTAGE): sensor.sensor_schema(
    unit_of_measurement=UNIT_PERCENT,
    accuracy_decimals=1,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_FAN_EXHAUST_PERCENTAGE): sensor.sensor_schema(
    unit_of_measurement=UNIT_PERCENT,
    accuracy_decimals=1,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_FAN_SPEED_SUPPLY): sensor.sensor_schema(
    unit_of_measurement="rpm",
    accuracy_decimals=0,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_FAN_SPEED_EXHAUST): sensor.sensor_schema(
    unit_of_measurement="rpm",
    accuracy_decimals=0,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_OUTSIDE_AIR_TEMPERATURE): sensor.sensor_schema(
    device_class=DEVICE_CLASS_TEMPERATURE,
    unit_of_measurement=UNIT_CELSIUS,
    accuracy_decimals=1,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_SUPPLY_AIR_TEMPERATURE): sensor.sensor_schema(
    device_class=DEVICE_CLASS_TEMPERATURE,
    unit_of_measurement=UNIT_CELSIUS,
    accuracy_decimals=1,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_RETURN_AIR_TEMPERATURE): sensor.sensor_schema(
    device_class=DEVICE_CLASS_TEMPERATURE,
    unit_of_measurement=UNIT_CELSIUS,
    accuracy_decimals=1,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_EXHAUST_AIR_TEMPERATURE): sensor.sensor_schema(
    device_class=DEVICE_CLASS_TEMPERATURE,
    unit_of_measurement=UNIT_CELSIUS,
    accuracy_decimals=1,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_ENTHALPY_TEMPERATURE): sensor.sensor_schema(
    device_class=DEVICE_CLASS_TEMPERATURE,
    unit_of_measurement=UNIT_CELSIUS,
    accuracy_decimals=1,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_EWT_TEMPERATURE): sensor.sensor_schema(
    device_class=DEVICE_CLASS_TEMPERATURE,
    unit_of_measurement=UNIT_CELSIUS,
    accuracy_decimals=1,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_REHEATING_TEMPERATURE): sensor.sensor_schema(
    device_class=DEVICE_CLASS_TEMPERATURE,
    unit_of_measurement=UNIT_CELSIUS,
    accuracy_decimals=1,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_KITCHEN_HOOD_TEMPERATURE): sensor.sensor_schema(
    device_class=DEVICE_CLASS_TEMPERATURE,
    unit_of_measurement=UNIT_CELSIUS,
    accuracy_decimals=1,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_RETURN_AIR_LEVEL): sensor.sensor_schema(
    device_class=DEVICE_CLASS_VOLUME,
    unit_of_measurement=UNIT_CUBIC_METER,
    accuracy_decimals=1,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_SUPPLY_AIR_LEVEL): sensor.sensor_schema(
    device_class=DEVICE_CLASS_VOLUME,
    unit_of_measurement=UNIT_CUBIC_METER,
    accuracy_decimals=1,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_BYPASS_FACTOR): sensor.sensor_schema(
    device_class=DEVICE_CLASS_VOLUME,
    unit_of_measurement=UNIT_PERCENT,
    accuracy_decimals=1,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_BYPASS_STEP): sensor.sensor_schema(
    device_class=DEVICE_CLASS_VOLUME,
    unit_of_measurement=UNIT_PERCENT,
    accuracy_decimals=1,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_BYPASS_CORRECTION): sensor.sensor_schema(
    device_class=DEVICE_CLASS_VOLUME,
    unit_of_measurement=UNIT_PERCENT,
    accuracy_decimals=1,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_IS_BYPASS_VALVE_OPEN): binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_EMPTY).extend(),
cv.Optional(CONF_IS_PREHEATING): binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_EMPTY).extend(),
cv.Optional(CONF_IS_SUMMER_MODE): binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_EMPTY).extend(),
cv.Optional(CONF_IS_SUPPLY_FAN_ACTIVE): binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_EMPTY).extend(),
cv.Optional(CONF_IS_FILTER_FULL): binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_EMPTY).extend(),
cv.Optional(CONF_FAN_HOURS_SUPPLY): sensor.sensor_schema(
    unit_of_measurement="h",
    accuracy_decimals=0,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_FAN_HOURS_EXHAUST): sensor.sensor_schema(
    unit_of_measurement="h",
    accuracy_decimals=0,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_BYPASS_HOURS): sensor.sensor_schema(
    unit_of_measurement="h",
    accuracy_decimals=0,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_PREHEATER_HOURS): sensor.sensor_schema(
    unit_of_measurement="h",
    accuracy_decimals=0,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_FILTER_HOURS): sensor.sensor_schema(
    unit_of_measurement="h",
    accuracy_decimals=0,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_EWT_HOURS): sensor.sensor_schema(
    unit_of_measurement="h",
    accuracy_decimals=0,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
cv.Optional(CONF_ERROR_CODE): sensor.sensor_schema(
    accuracy_decimals=0,
    state_class=STATE_CLASS_MEASUREMENT).extend(),
})

CONFIG_SCHEMA = cv.All(
    cv.Schema( {
        cv.GenerateID(CONF_ID): cv.declare_id(ComfoAirComponent),
    })
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(comfoair_sensors_schemas)
    .extend(cv.COMPONENT_SCHEMA)
    .extend(climate.climate_schema(ComfoAirComponent))
    .extend({
        cv.Optional(CONF_AUTO_BALANCE): switch.switch_schema(ComfoAirAutoBalanceSwitch),
    })
)


async def to_code(config):
    """Generates code"""
    # var = cg.new_Pvariable(config[CONF_ID])
    var = await climate.new_climate(config)
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    paren = await cg.get_variable(config[CONF_UART_ID])
    cg.add(var.set_uart_component(paren))
    for k in helper_comfoair_list:
        if k in config:
            sens = None
            if 'is_' in k:
                sens = await binary_sensor.new_binary_sensor(config[k])
            else:
                sens = await sensor.new_sensor(config[k])
            func = getattr(var, 'set_'+k)
            cg.add(func(sens))
    
    if CONF_AUTO_BALANCE in config:
        sw = cg.new_Pvariable(config[CONF_AUTO_BALANCE][CONF_ID])
        await cg.register_component(sw, config[CONF_AUTO_BALANCE])
        await switch.register_switch(sw, config[CONF_AUTO_BALANCE])
        cg.add(sw.set_parent(var))
        cg.add(var.set_auto_balance_switch(sw))
