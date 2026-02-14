import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv

from . import CONF_LITIME_BMS_BLE_ID, LITIME_BMS_BLE_COMPONENT_SCHEMA

DEPENDENCIES = ["litime_bms_ble"]

CONF_CHARGING = "charging"
CONF_DISCHARGING = "discharging"
CONF_BALANCING = "balancing"
CONF_ONLINE = "online"

BINARY_SENSORS = {
    CONF_CHARGING: binary_sensor.binary_sensor_schema(
        icon="mdi:battery-charging",
    ),
    CONF_DISCHARGING: binary_sensor.binary_sensor_schema(
        icon="mdi:battery-arrow-down-outline",
    ),
    CONF_BALANCING: binary_sensor.binary_sensor_schema(
        icon="mdi:battery-sync",
    ),
    CONF_ONLINE: binary_sensor.binary_sensor_schema(
        device_class="connectivity",
    ),
}

CONFIG_SCHEMA = LITIME_BMS_BLE_COMPONENT_SCHEMA.extend(
    {cv.Optional(key): schema for key, schema in BINARY_SENSORS.items()}
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_LITIME_BMS_BLE_ID])

    for key in BINARY_SENSORS:
        if key in config:
            sens = await binary_sensor.new_binary_sensor(config[key])
            cg.add(getattr(hub, f"set_{key}_binary_sensor")(sens))
