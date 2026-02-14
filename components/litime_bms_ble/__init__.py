import esphome.codegen as cg
from esphome.components import ble_client, esp32_ble_tracker
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@rubenmuehlhans"]
DEPENDENCIES = ["esp32_ble_tracker"]
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor", "switch"]
MULTI_CONF = True

CONF_LITIME_BMS_BLE_ID = "litime_bms_ble_id"
CONF_BLE_CLIENT_ID = "ble_client_id"

litime_bms_ble_ns = cg.esphome_ns.namespace("litime_bms_ble")

LitimeBmsBle = litime_bms_ble_ns.class_(
    "LitimeBmsBle",
    ble_client.BLEClientNode,
    cg.PollingComponent,
)

LitimeBleScanner = litime_bms_ble_ns.class_(
    "LitimeBleScanner",
    esp32_ble_tracker.ESPBTDeviceListener,
    cg.Component,
)

# Schema that child platforms (sensor, switch, etc.) use to reference the hub
LITIME_BMS_BLE_COMPONENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_LITIME_BMS_BLE_ID): cv.use_id(LitimeBmsBle),
    }
)

# Monitor mode schema: connects to a specific battery via ble_client
MONITOR_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LitimeBmsBle),
        }
    )
    .extend(ble_client.BLE_CLIENT_SCHEMA)
    .extend(cv.polling_component_schema("5s"))
)

# Scanner mode schema: passively listens for LiTime BLE advertisements
SCANNER_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LitimeBleScanner),
        }
    )
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


def _detect_mode(config):
    """Auto-detect mode based on presence of ble_client_id."""
    if CONF_BLE_CLIENT_ID in config:
        return MONITOR_SCHEMA(config)
    return SCANNER_SCHEMA(config)


CONFIG_SCHEMA = _detect_mode


async def to_code(config):
    if CONF_BLE_CLIENT_ID in config:
        # Monitor mode: BLE client connected to a specific battery
        var = cg.new_Pvariable(config[CONF_ID])
        await cg.register_component(var, config)
        await ble_client.register_ble_node(var, config)
    else:
        # Scanner mode: passive BLE advertisement listener
        var = cg.new_Pvariable(config[CONF_ID])
        await cg.register_component(var, config)
        await esp32_ble_tracker.register_ble_device(var, config)
