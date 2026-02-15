#pragma once

#include "esphome/core/component.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/switch/switch.h"

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#ifdef USE_ESP32

namespace esphome {
namespace litime_bms_ble {

namespace espbt = esphome::esp32_ble_tracker;

// BLE UUIDs for LiTime BMS
static const espbt::ESPBTUUID SERVICE_UUID =
    espbt::ESPBTUUID::from_raw("0000ffe0-0000-1000-8000-00805f9b34fb");
static const espbt::ESPBTUUID NOTIFY_CHAR_UUID =
    espbt::ESPBTUUID::from_raw("0000ffe1-0000-1000-8000-00805f9b34fb");
static const espbt::ESPBTUUID WRITE_CHAR_UUID =
    espbt::ESPBTUUID::from_raw("0000ffe2-0000-1000-8000-00805f9b34fb");

// Command IDs
static const uint8_t CMD_REGISTER = 0x01;
static const uint8_t CMD_DISCONNECT = 0x02;
static const uint8_t CMD_CHARGE_ON = 0x0A;
static const uint8_t CMD_CHARGE_OFF = 0x0B;
static const uint8_t CMD_DISCHARGE_ON = 0x0C;
static const uint8_t CMD_DISCHARGE_OFF = 0x0D;
static const uint8_t CMD_SERIAL_NUMBER = 0x10;
static const uint8_t CMD_QUERY_STATUS = 0x13;
static const uint8_t CMD_FIRMWARE_VERSION = 0x16;
static const uint8_t CMD_GET_SOH_SOC = 0x41;
static const uint8_t CMD_GET_NOMINAL_CAPACITY = 0x43;

// Maximum number of cells supported
static const uint8_t MAX_CELLS = 16;

// Minimum response length for status query
static const size_t MIN_RESPONSE_LENGTH = 104;

// Protection state flags
static const uint32_t PROTECTION_OVERCHARGE = 0x00000004;
static const uint32_t PROTECTION_OVERDISCHARGE = 0x00000020;
static const uint32_t PROTECTION_CHARGE_OVERCURRENT = 0x00000040;
static const uint32_t PROTECTION_DISCHARGE_OVERCURRENT = 0x00000080;
static const uint32_t PROTECTION_HIGH_TEMP_1 = 0x00000100;
static const uint32_t PROTECTION_HIGH_TEMP_2 = 0x00000200;
static const uint32_t PROTECTION_LOW_TEMP_1 = 0x00000400;
static const uint32_t PROTECTION_LOW_TEMP_2 = 0x00000800;
static const uint32_t PROTECTION_SHORT_CIRCUIT = 0x00004000;

// Battery state values
static const uint16_t BATTERY_STATE_DISCHARGING = 0x0000;
static const uint16_t BATTERY_STATE_CHARGING = 0x0001;
static const uint16_t BATTERY_STATE_CHARGE_DISABLED = 0x0004;

// ============================================================================
// LiTime BMS BLE Monitor
// ============================================================================

class LitimeBmsBle : public ble_client::BLEClientNode, public PollingComponent {
 public:
  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;
  void dump_config() override;
  void update() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // Sensor setters — called by sensor.py codegen
  void set_total_voltage_sensor(sensor::Sensor *s) { total_voltage_sensor_ = s; }
  void set_current_sensor(sensor::Sensor *s) { current_sensor_ = s; }
  void set_power_sensor(sensor::Sensor *s) { power_sensor_ = s; }
  void set_state_of_charge_sensor(sensor::Sensor *s) { state_of_charge_sensor_ = s; }
  void set_state_of_health_sensor(sensor::Sensor *s) { state_of_health_sensor_ = s; }
  void set_cell_temperature_sensor(sensor::Sensor *s) { cell_temperature_sensor_ = s; }
  void set_mosfet_temperature_sensor(sensor::Sensor *s) { mosfet_temperature_sensor_ = s; }
  void set_remaining_capacity_sensor(sensor::Sensor *s) { remaining_capacity_sensor_ = s; }
  void set_full_charge_capacity_sensor(sensor::Sensor *s) { full_charge_capacity_sensor_ = s; }
  void set_discharge_cycles_sensor(sensor::Sensor *s) { discharge_cycles_sensor_ = s; }
  void set_total_discharge_ah_sensor(sensor::Sensor *s) { total_discharge_ah_sensor_ = s; }
  void set_min_cell_voltage_sensor(sensor::Sensor *s) { min_cell_voltage_sensor_ = s; }
  void set_max_cell_voltage_sensor(sensor::Sensor *s) { max_cell_voltage_sensor_ = s; }
  void set_delta_cell_voltage_sensor(sensor::Sensor *s) { delta_cell_voltage_sensor_ = s; }

  void set_cell_voltage_sensor(uint8_t cell, sensor::Sensor *s) {
    if (cell < MAX_CELLS)
      cell_voltage_sensors_[cell] = s;
  }

  // Binary sensor setters
  void set_charging_binary_sensor(binary_sensor::BinarySensor *s) { charging_binary_sensor_ = s; }
  void set_discharging_binary_sensor(binary_sensor::BinarySensor *s) { discharging_binary_sensor_ = s; }
  void set_balancing_binary_sensor(binary_sensor::BinarySensor *s) { balancing_binary_sensor_ = s; }
  void set_online_binary_sensor(binary_sensor::BinarySensor *s) { online_binary_sensor_ = s; }

  // Text sensor setters
  void set_protection_status_text_sensor(text_sensor::TextSensor *s) { protection_status_text_sensor_ = s; }
  void set_failure_status_text_sensor(text_sensor::TextSensor *s) { failure_status_text_sensor_ = s; }

  // Switch control methods — called by switch.py
  void set_charging_enabled(bool enabled);
  void set_discharging_enabled(bool enabled);

  // Switch object setters — for state feedback
  void set_charging_switch(switch_::Switch *s) { charging_switch_ = s; }
  void set_discharging_switch(switch_::Switch *s) { discharging_switch_ = s; }

 protected:
  // Send a command to the BMS
  void send_command_(uint8_t cmd);

  // Parse the response from status query (cmd 0x13)
  void parse_status_response_(const uint8_t *data, size_t len);

  // Decode protection flags to human-readable string
  std::string decode_protection_flags_(uint32_t flags);

  // Decode failure flags to human-readable string
  std::string decode_failure_flags_(uint32_t flags);

  // Publish a sensor value (with null check)
  void publish_state_(sensor::Sensor *sensor, float value);
  void publish_state_(binary_sensor::BinarySensor *sensor, bool value);
  void publish_state_(text_sensor::TextSensor *sensor, const std::string &value);

  // Set all sensors to NAN (offline)
  void publish_offline_();

  // BLE characteristic handles
  uint16_t char_notify_handle_{0};
  uint16_t char_write_handle_{0};

  // Missed update counter for offline detection
  uint8_t missed_updates_{0};
  static const uint8_t MAX_MISSED_UPDATES = 5;

  // Whether we received a response since last update()
  bool response_received_{false};

  // Sensor pointers
  sensor::Sensor *total_voltage_sensor_{nullptr};
  sensor::Sensor *current_sensor_{nullptr};
  sensor::Sensor *power_sensor_{nullptr};
  sensor::Sensor *state_of_charge_sensor_{nullptr};
  sensor::Sensor *state_of_health_sensor_{nullptr};
  sensor::Sensor *cell_temperature_sensor_{nullptr};
  sensor::Sensor *mosfet_temperature_sensor_{nullptr};
  sensor::Sensor *remaining_capacity_sensor_{nullptr};
  sensor::Sensor *full_charge_capacity_sensor_{nullptr};
  sensor::Sensor *discharge_cycles_sensor_{nullptr};
  sensor::Sensor *total_discharge_ah_sensor_{nullptr};
  sensor::Sensor *min_cell_voltage_sensor_{nullptr};
  sensor::Sensor *max_cell_voltage_sensor_{nullptr};
  sensor::Sensor *delta_cell_voltage_sensor_{nullptr};
  sensor::Sensor *cell_voltage_sensors_[MAX_CELLS]{nullptr};

  // Binary sensor pointers
  binary_sensor::BinarySensor *charging_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *discharging_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *balancing_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *online_binary_sensor_{nullptr};

  // Text sensor pointers
  text_sensor::TextSensor *protection_status_text_sensor_{nullptr};
  text_sensor::TextSensor *failure_status_text_sensor_{nullptr};

  // Switch pointers (for state feedback)
  switch_::Switch *charging_switch_{nullptr};
  switch_::Switch *discharging_switch_{nullptr};
};

// ============================================================================
// Switch classes — forward write_state to the hub
// ============================================================================

class LitimeChargingSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(LitimeBmsBle *parent) { this->parent_ = parent; }
  void dump_config() override;

 protected:
  void write_state(bool state) override {
    this->parent_->set_charging_enabled(state);
    this->publish_state(state);
  }
  LitimeBmsBle *parent_{nullptr};
};

class LitimeDischargingSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(LitimeBmsBle *parent) { this->parent_ = parent; }
  void dump_config() override;

 protected:
  void write_state(bool state) override {
    this->parent_->set_discharging_enabled(state);
    this->publish_state(state);
  }
  LitimeBmsBle *parent_{nullptr};
};

class LitimeConnectSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(LitimeBmsBle *parent) { this->parent_ = parent; }
  void dump_config() override;

 protected:
  void write_state(bool state) override {
    if (this->parent_->parent() != nullptr) {
      this->parent_->parent()->set_enabled(state);
    }
    this->publish_state(state);
  }
  LitimeBmsBle *parent_{nullptr};
};

// ============================================================================
// LiTime BLE Scanner — passive discovery of LiTime devices
// ============================================================================

class LitimeBleScanner : public esp32_ble_tracker::ESPBTDeviceListener, public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  bool parse_device(const espbt::ESPBTDevice &device) override;

 protected:
  // Track already-discovered MAC addresses to avoid duplicate log spam
  std::set<uint64_t> discovered_devices_;
};

}  // namespace litime_bms_ble
}  // namespace esphome

#endif  // USE_ESP32
