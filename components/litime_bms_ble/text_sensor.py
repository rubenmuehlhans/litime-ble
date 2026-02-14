import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv

from . import CONF_LITIME_BMS_BLE_ID, LITIME_BMS_BLE_COMPONENT_SCHEMA

DEPENDENCIES = ["litime_bms_ble"]

CONF_PROTECTION_STATUS = "protection_status"
CONF_FAILURE_STATUS = "failure_status"

TEXT_SENSORS = {
    CONF_PROTECTION_STATUS: text_sensor.text_sensor_schema(
        icon="mdi:shield-alert",
    ),
    CONF_FAILURE_STATUS: text_sensor.text_sensor_schema(
        icon="mdi:alert-circle",
    ),
}

CONFIG_SCHEMA = LITIME_BMS_BLE_COMPONENT_SCHEMA.extend(
    {cv.Optional(key): schema for key, schema in TEXT_SENSORS.items()}
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_LITIME_BMS_BLE_ID])

    for key in TEXT_SENSORS:
        if key in config:
            sens = await text_sensor.new_text_sensor(config[key])
            cg.add(getattr(hub, f"set_{key}_text_sensor")(sens))
