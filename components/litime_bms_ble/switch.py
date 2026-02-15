import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import CONF_ID

from . import CONF_LITIME_BMS_BLE_ID, LITIME_BMS_BLE_COMPONENT_SCHEMA, litime_bms_ble_ns, LitimeBmsBle

DEPENDENCIES = ["litime_bms_ble"]

CONF_CHARGING_SWITCH = "charging_switch"
CONF_DISCHARGING_SWITCH = "discharging_switch"
CONF_CONNECT_SWITCH = "connect_switch"

# Custom switch classes that forward write_state to the hub
LitimeChargingSwitch = litime_bms_ble_ns.class_(
    "LitimeChargingSwitch", switch.Switch, cg.Component
)

LitimeDischargingSwitch = litime_bms_ble_ns.class_(
    "LitimeDischargingSwitch", switch.Switch, cg.Component
)

LitimeConnectSwitch = litime_bms_ble_ns.class_(
    "LitimeConnectSwitch", switch.Switch, cg.Component
)

CONFIG_SCHEMA = LITIME_BMS_BLE_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(CONF_CHARGING_SWITCH): switch.switch_schema(
            LitimeChargingSwitch,
            icon="mdi:battery-charging",
        ),
        cv.Optional(CONF_DISCHARGING_SWITCH): switch.switch_schema(
            LitimeDischargingSwitch,
            icon="mdi:battery-arrow-down-outline",
        ),
        cv.Optional(CONF_CONNECT_SWITCH): switch.switch_schema(
            LitimeConnectSwitch,
            icon="mdi:bluetooth-connect",
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_LITIME_BMS_BLE_ID])

    if CONF_CHARGING_SWITCH in config:
        sw = await switch.new_switch(config[CONF_CHARGING_SWITCH])
        await cg.register_component(sw, config[CONF_CHARGING_SWITCH])
        cg.add(sw.set_parent(hub))
        cg.add(hub.set_charging_switch(sw))

    if CONF_DISCHARGING_SWITCH in config:
        sw = await switch.new_switch(config[CONF_DISCHARGING_SWITCH])
        await cg.register_component(sw, config[CONF_DISCHARGING_SWITCH])
        cg.add(sw.set_parent(hub))
        cg.add(hub.set_discharging_switch(sw))

    if CONF_CONNECT_SWITCH in config:
        sw = await switch.new_switch(config[CONF_CONNECT_SWITCH])
        await cg.register_component(sw, config[CONF_CONNECT_SWITCH])
        cg.add(sw.set_parent(hub))
