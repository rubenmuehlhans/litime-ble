import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_CURRENT,
    CONF_POWER,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_AMPERE,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    UNIT_VOLT,
    UNIT_WATT,
)

from . import CONF_LITIME_BMS_BLE_ID, LITIME_BMS_BLE_COMPONENT_SCHEMA

DEPENDENCIES = ["litime_bms_ble"]

UNIT_AMPERE_HOURS = "Ah"

CONF_TOTAL_VOLTAGE = "total_voltage"
CONF_STATE_OF_CHARGE = "state_of_charge"
CONF_STATE_OF_HEALTH = "state_of_health"
CONF_CELL_TEMPERATURE = "cell_temperature"
CONF_MOSFET_TEMPERATURE = "mosfet_temperature"
CONF_REMAINING_CAPACITY = "remaining_capacity"
CONF_FULL_CHARGE_CAPACITY = "full_charge_capacity"
CONF_DISCHARGE_CYCLES = "discharge_cycles"
CONF_TOTAL_DISCHARGE_AH = "total_discharge_ah"
CONF_MIN_CELL_VOLTAGE = "min_cell_voltage"
CONF_MAX_CELL_VOLTAGE = "max_cell_voltage"
CONF_DELTA_CELL_VOLTAGE = "delta_cell_voltage"

# Individual cell voltages
CONF_CELL_VOLTAGE_1 = "cell_voltage_1"
CONF_CELL_VOLTAGE_2 = "cell_voltage_2"
CONF_CELL_VOLTAGE_3 = "cell_voltage_3"
CONF_CELL_VOLTAGE_4 = "cell_voltage_4"
CONF_CELL_VOLTAGE_5 = "cell_voltage_5"
CONF_CELL_VOLTAGE_6 = "cell_voltage_6"
CONF_CELL_VOLTAGE_7 = "cell_voltage_7"
CONF_CELL_VOLTAGE_8 = "cell_voltage_8"
CONF_CELL_VOLTAGE_9 = "cell_voltage_9"
CONF_CELL_VOLTAGE_10 = "cell_voltage_10"
CONF_CELL_VOLTAGE_11 = "cell_voltage_11"
CONF_CELL_VOLTAGE_12 = "cell_voltage_12"
CONF_CELL_VOLTAGE_13 = "cell_voltage_13"
CONF_CELL_VOLTAGE_14 = "cell_voltage_14"
CONF_CELL_VOLTAGE_15 = "cell_voltage_15"
CONF_CELL_VOLTAGE_16 = "cell_voltage_16"

CELL_VOLTAGE_KEYS = [
    CONF_CELL_VOLTAGE_1,
    CONF_CELL_VOLTAGE_2,
    CONF_CELL_VOLTAGE_3,
    CONF_CELL_VOLTAGE_4,
    CONF_CELL_VOLTAGE_5,
    CONF_CELL_VOLTAGE_6,
    CONF_CELL_VOLTAGE_7,
    CONF_CELL_VOLTAGE_8,
    CONF_CELL_VOLTAGE_9,
    CONF_CELL_VOLTAGE_10,
    CONF_CELL_VOLTAGE_11,
    CONF_CELL_VOLTAGE_12,
    CONF_CELL_VOLTAGE_13,
    CONF_CELL_VOLTAGE_14,
    CONF_CELL_VOLTAGE_15,
    CONF_CELL_VOLTAGE_16,
]

# Sensors with simple set_X_sensor() setters
SIMPLE_SENSORS = {
    CONF_TOTAL_VOLTAGE: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_CURRENT: sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_CURRENT,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_POWER: sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_STATE_OF_CHARGE: sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_BATTERY,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_STATE_OF_HEALTH: sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        accuracy_decimals=0,
        icon="mdi:heart-pulse",
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_CELL_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_MOSFET_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_REMAINING_CAPACITY: sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE_HOURS,
        accuracy_decimals=2,
        icon="mdi:battery-arrow-down",
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_FULL_CHARGE_CAPACITY: sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE_HOURS,
        accuracy_decimals=2,
        icon="mdi:battery-arrow-up",
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_DISCHARGE_CYCLES: sensor.sensor_schema(
        accuracy_decimals=0,
        icon="mdi:counter",
        state_class=STATE_CLASS_TOTAL_INCREASING,
    ),
    CONF_TOTAL_DISCHARGE_AH: sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE_HOURS,
        accuracy_decimals=1,
        icon="mdi:counter",
        state_class=STATE_CLASS_TOTAL_INCREASING,
    ),
    CONF_MIN_CELL_VOLTAGE: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_MAX_CELL_VOLTAGE: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_DELTA_CELL_VOLTAGE: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
}

# Build cell voltage sensor schemas
CELL_VOLTAGE_SCHEMAS = {}
for key in CELL_VOLTAGE_KEYS:
    CELL_VOLTAGE_SCHEMAS[key] = sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
    )

# Combined config schema
CONFIG_SCHEMA = LITIME_BMS_BLE_COMPONENT_SCHEMA.extend(
    {cv.Optional(key): schema for key, schema in SIMPLE_SENSORS.items()}
).extend(
    {cv.Optional(key): schema for key, schema in CELL_VOLTAGE_SCHEMAS.items()}
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_LITIME_BMS_BLE_ID])

    # Register simple sensors
    for key in SIMPLE_SENSORS:
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(getattr(hub, f"set_{key}_sensor")(sens))

    # Register cell voltage sensors
    for i, key in enumerate(CELL_VOLTAGE_KEYS):
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(hub.set_cell_voltage_sensor(i, sens))
