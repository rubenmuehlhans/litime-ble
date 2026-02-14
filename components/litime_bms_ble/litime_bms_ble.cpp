#include "litime_bms_ble.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#ifdef USE_ESP32

namespace esphome {
namespace litime_bms_ble {

static const char *const TAG = "litime_bms_ble";

// ============================================================================
// Helper: build an 8-byte command frame
// Format: {0x00, 0x00, 0x04, 0x01, CMD, 0x55, 0xAA, CHECKSUM}
// Checksum = byte[2] + byte[3] + byte[4] = 0x04 + 0x01 + CMD = 0x05 + CMD
// ============================================================================
static std::vector<uint8_t> build_command(uint8_t cmd) {
  uint8_t checksum = 0x04 + 0x01 + cmd;
  return {0x00, 0x00, 0x04, 0x01, cmd, 0x55, 0xAA, checksum};
}

// ============================================================================
// Helper: read little-endian values from a byte buffer
// ============================================================================
static uint16_t get_uint16_le(const uint8_t *data) {
  return (static_cast<uint16_t>(data[1]) << 8) | data[0];
}

static int16_t get_int16_le(const uint8_t *data) {
  return static_cast<int16_t>(get_uint16_le(data));
}

static uint32_t get_uint32_le(const uint8_t *data) {
  return (static_cast<uint32_t>(data[3]) << 24) |
         (static_cast<uint32_t>(data[2]) << 16) |
         (static_cast<uint32_t>(data[1]) << 8) |
         data[0];
}

static int32_t get_int32_le(const uint8_t *data) {
  return static_cast<int32_t>(get_uint32_le(data));
}

// ============================================================================
// LitimeBmsBle — Monitor mode
// ============================================================================

void LitimeBmsBle::dump_config() {
  ESP_LOGCONFIG(TAG, "LiTime BMS BLE Monitor:");
  ESP_LOGCONFIG(TAG, "  Update Interval: %ums", this->get_update_interval());
  LOG_SENSOR("  ", "Total Voltage", this->total_voltage_sensor_);
  LOG_SENSOR("  ", "Current", this->current_sensor_);
  LOG_SENSOR("  ", "Power", this->power_sensor_);
  LOG_SENSOR("  ", "State of Charge", this->state_of_charge_sensor_);
  LOG_SENSOR("  ", "State of Health", this->state_of_health_sensor_);
  LOG_SENSOR("  ", "Cell Temperature", this->cell_temperature_sensor_);
  LOG_SENSOR("  ", "MOSFET Temperature", this->mosfet_temperature_sensor_);
  LOG_SENSOR("  ", "Remaining Capacity", this->remaining_capacity_sensor_);
  LOG_SENSOR("  ", "Full Charge Capacity", this->full_charge_capacity_sensor_);
  LOG_SENSOR("  ", "Discharge Cycles", this->discharge_cycles_sensor_);
  LOG_SENSOR("  ", "Total Discharge Ah", this->total_discharge_ah_sensor_);
  LOG_SENSOR("  ", "Min Cell Voltage", this->min_cell_voltage_sensor_);
  LOG_SENSOR("  ", "Max Cell Voltage", this->max_cell_voltage_sensor_);
  LOG_SENSOR("  ", "Delta Cell Voltage", this->delta_cell_voltage_sensor_);
  for (uint8_t i = 0; i < MAX_CELLS; i++) {
    if (this->cell_voltage_sensors_[i] != nullptr) {
      ESP_LOGCONFIG(TAG, "  Cell Voltage %d: configured", i + 1);
    }
  }
  LOG_BINARY_SENSOR("  ", "Charging", this->charging_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Discharging", this->discharging_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Balancing", this->balancing_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Online", this->online_binary_sensor_);
  LOG_TEXT_SENSOR("  ", "Protection Status", this->protection_status_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Failure Status", this->failure_status_text_sensor_);
}

void LitimeBmsBle::gattc_event_handler(esp_gattc_cb_event_t event,
                                        esp_gatt_if_t gattc_if,
                                        esp_ble_gattc_cb_param_t *param) {
  switch (event) {
    case ESP_GATTC_DISCONNECT_EVT: {
      ESP_LOGI(TAG, "BLE disconnected");
      this->node_state = espbt::ClientState::IDLE;
      this->char_notify_handle_ = 0;
      this->char_write_handle_ = 0;
      this->publish_state_(this->online_binary_sensor_, false);
      break;
    }

    case ESP_GATTC_SEARCH_CMPL_EVT: {
      // Service discovery complete — find our characteristics
      auto *chr_notify = this->parent()->get_characteristic(SERVICE_UUID, NOTIFY_CHAR_UUID);
      if (chr_notify == nullptr) {
        ESP_LOGE(TAG, "Could not find notify characteristic (FFE1)");
        break;
      }
      this->char_notify_handle_ = chr_notify->handle;

      auto *chr_write = this->parent()->get_characteristic(SERVICE_UUID, WRITE_CHAR_UUID);
      if (chr_write == nullptr) {
        ESP_LOGE(TAG, "Could not find write characteristic (FFE2)");
        break;
      }
      this->char_write_handle_ = chr_write->handle;

      // Register for notifications on FFE1
      auto status = esp_ble_gattc_register_for_notify(
          this->parent()->get_gattc_if(),
          this->parent()->get_remote_bda(),
          chr_notify->handle);
      if (status != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register for notifications: %d", status);
      } else {
        ESP_LOGI(TAG, "Registered for BLE notifications");
      }
      break;
    }

    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
      // Notification registration complete — connection is established
      this->node_state = espbt::ClientState::ESTABLISHED;
      this->missed_updates_ = 0;
      ESP_LOGI(TAG, "BLE connection established");
      this->publish_state_(this->online_binary_sensor_, true);

      // Send initial registration command
      this->send_command_(CMD_REGISTER);
      break;
    }

    case ESP_GATTC_NOTIFY_EVT: {
      // Data received from BMS
      if (param->notify.handle != this->char_notify_handle_)
        break;

      ESP_LOGV(TAG, "Received notification: %d bytes", param->notify.value_len);

      // Check if this is a status response (must be at least MIN_RESPONSE_LENGTH bytes)
      if (param->notify.value_len >= MIN_RESPONSE_LENGTH) {
        this->parse_status_response_(param->notify.value, param->notify.value_len);
        this->response_received_ = true;
        this->missed_updates_ = 0;
      } else {
        ESP_LOGD(TAG, "Received short response (%d bytes), ignoring",
                 param->notify.value_len);
      }
      break;
    }

    default:
      break;
  }
}

void LitimeBmsBle::update() {
  if (this->node_state != espbt::ClientState::ESTABLISHED) {
    ESP_LOGD(TAG, "Not connected, skipping update");
    return;
  }

  // Check for missed responses
  if (!this->response_received_) {
    this->missed_updates_++;
    if (this->missed_updates_ >= MAX_MISSED_UPDATES) {
      ESP_LOGW(TAG, "No response for %d updates, publishing offline", this->missed_updates_);
      this->publish_offline_();
      return;
    }
  }
  this->response_received_ = false;

  // Send status query command
  this->send_command_(CMD_QUERY_STATUS);
}

void LitimeBmsBle::send_command_(uint8_t cmd) {
  if (this->char_write_handle_ == 0) {
    ESP_LOGW(TAG, "Write characteristic not available");
    return;
  }

  auto frame = build_command(cmd);

  ESP_LOGD(TAG, "Sending command 0x%02X (%d bytes)", cmd, frame.size());

  auto status = esp_ble_gattc_write_char(
      this->parent()->get_gattc_if(),
      this->parent()->get_conn_id(),
      this->char_write_handle_,
      frame.size(),
      frame.data(),
      ESP_GATT_WRITE_TYPE_NO_RSP,
      ESP_GATT_AUTH_REQ_NONE);

  if (status != ESP_OK) {
    ESP_LOGW(TAG, "Failed to send command 0x%02X: %d", cmd, status);
  }
}

void LitimeBmsBle::parse_status_response_(const uint8_t *data, size_t len) {
  ESP_LOGD(TAG, "Parsing status response (%d bytes)", len);

  // --- Total voltage (bytes 8-11) ---
  float total_voltage = get_uint32_le(data + 8) / 1000.0f;
  this->publish_state_(this->total_voltage_sensor_, total_voltage);

  // --- Individual cell voltages (bytes 16-47, 16x uint16_le) ---
  float min_cell = 99.0f;
  float max_cell = 0.0f;
  uint8_t cell_count = 0;

  for (uint8_t i = 0; i < MAX_CELLS; i++) {
    uint16_t raw = get_uint16_le(data + 16 + (i * 2));
    if (raw == 0)
      continue;  // Cell not present

    float cell_v = raw / 1000.0f;
    cell_count++;

    this->publish_state_(this->cell_voltage_sensors_[i], cell_v);

    if (cell_v < min_cell)
      min_cell = cell_v;
    if (cell_v > max_cell)
      max_cell = cell_v;
  }

  if (cell_count > 0) {
    this->publish_state_(this->min_cell_voltage_sensor_, min_cell);
    this->publish_state_(this->max_cell_voltage_sensor_, max_cell);
    this->publish_state_(this->delta_cell_voltage_sensor_, max_cell - min_cell);
  }

  // --- Current (bytes 48-51, int32_le / 1000) ---
  float current = get_int32_le(data + 48) / 1000.0f;
  this->publish_state_(this->current_sensor_, current);

  // --- Power (calculated) ---
  this->publish_state_(this->power_sensor_, total_voltage * current);

  // --- Cell temperature (bytes 52-53, int16_le) ---
  float cell_temp = static_cast<float>(get_int16_le(data + 52));
  this->publish_state_(this->cell_temperature_sensor_, cell_temp);

  // --- MOSFET temperature (bytes 54-55, int16_le) ---
  float mosfet_temp = static_cast<float>(get_int16_le(data + 54));
  this->publish_state_(this->mosfet_temperature_sensor_, mosfet_temp);

  // --- Remaining capacity (bytes 62-63, uint16_le / 100) ---
  float remaining_ah = get_uint16_le(data + 62) / 100.0f;
  this->publish_state_(this->remaining_capacity_sensor_, remaining_ah);

  // --- Full charge capacity (bytes 64-65, uint16_le / 100) ---
  float full_ah = get_uint16_le(data + 64) / 100.0f;
  this->publish_state_(this->full_charge_capacity_sensor_, full_ah);

  // --- Protection state (bytes 76-79, uint32_le) ---
  uint32_t protection_flags = get_uint32_le(data + 76);
  std::string protection_text = this->decode_protection_flags_(protection_flags);
  this->publish_state_(this->protection_status_text_sensor_, protection_text);

  // --- Failure state (bytes 80-83, uint32_le) ---
  uint32_t failure_flags = get_uint32_le(data + 80);
  std::string failure_text = this->decode_failure_flags_(failure_flags);
  this->publish_state_(this->failure_status_text_sensor_, failure_text);

  // --- Balancing state (bytes 84-87, uint32_le) ---
  uint32_t balancing_state = get_uint32_le(data + 84);
  this->publish_state_(this->balancing_binary_sensor_, balancing_state != 0);

  // --- Battery state (bytes 88-89, uint16_le) ---
  uint16_t battery_state = get_uint16_le(data + 88);
  this->publish_state_(this->charging_binary_sensor_, battery_state == BATTERY_STATE_CHARGING);
  this->publish_state_(this->discharging_binary_sensor_,
                       battery_state == BATTERY_STATE_DISCHARGING && current < 0);

  // --- SOC (bytes 90-91, uint16_le) ---
  float soc = static_cast<float>(get_uint16_le(data + 90));
  this->publish_state_(this->state_of_charge_sensor_, soc);

  // --- SOH (bytes 92-95, uint32_le) ---
  float soh = static_cast<float>(get_uint32_le(data + 92));
  this->publish_state_(this->state_of_health_sensor_, soh);

  // --- Discharge cycle count (bytes 96-99, uint32_le) ---
  float cycles = static_cast<float>(get_uint32_le(data + 96));
  this->publish_state_(this->discharge_cycles_sensor_, cycles);

  // --- Total discharge Ah (bytes 100-103, uint32_le / 1000) ---
  float total_ah = get_uint32_le(data + 100) / 1000.0f;
  this->publish_state_(this->total_discharge_ah_sensor_, total_ah);

  // --- Update switch states based on battery state ---
  // Heat state byte 68-71: bit 0x80 = discharge disabled
  uint32_t heat_state = get_uint32_le(data + 68);
  bool discharge_enabled = !(heat_state & 0x00000080);
  bool charge_enabled = (battery_state != BATTERY_STATE_CHARGE_DISABLED);

  if (this->charging_switch_ != nullptr) {
    this->charging_switch_->publish_state(charge_enabled);
  }
  if (this->discharging_switch_ != nullptr) {
    this->discharging_switch_->publish_state(discharge_enabled);
  }
}

std::string LitimeBmsBle::decode_protection_flags_(uint32_t flags) {
  if (flags == 0)
    return "OK";

  std::string result;
  if (flags & PROTECTION_OVERCHARGE) {
    if (!result.empty()) result += ", ";
    result += "Overcharge";
  }
  if (flags & PROTECTION_OVERDISCHARGE) {
    if (!result.empty()) result += ", ";
    result += "Over-discharge";
  }
  if (flags & PROTECTION_CHARGE_OVERCURRENT) {
    if (!result.empty()) result += ", ";
    result += "Charge overcurrent";
  }
  if (flags & PROTECTION_DISCHARGE_OVERCURRENT) {
    if (!result.empty()) result += ", ";
    result += "Discharge overcurrent";
  }
  if (flags & PROTECTION_HIGH_TEMP_1) {
    if (!result.empty()) result += ", ";
    result += "High temp 1";
  }
  if (flags & PROTECTION_HIGH_TEMP_2) {
    if (!result.empty()) result += ", ";
    result += "High temp 2";
  }
  if (flags & PROTECTION_LOW_TEMP_1) {
    if (!result.empty()) result += ", ";
    result += "Low temp 1";
  }
  if (flags & PROTECTION_LOW_TEMP_2) {
    if (!result.empty()) result += ", ";
    result += "Low temp 2";
  }
  if (flags & PROTECTION_SHORT_CIRCUIT) {
    if (!result.empty()) result += ", ";
    result += "Short circuit";
  }
  return result;
}

std::string LitimeBmsBle::decode_failure_flags_(uint32_t flags) {
  if (flags == 0)
    return "OK";

  // Failure flags are less well-documented; return hex for now
  char buf[32];
  snprintf(buf, sizeof(buf), "Error: 0x%08X", flags);
  return std::string(buf);
}

void LitimeBmsBle::set_charging_enabled(bool enabled) {
  ESP_LOGI(TAG, "Setting charging %s", enabled ? "ON" : "OFF");
  this->send_command_(enabled ? CMD_CHARGE_ON : CMD_CHARGE_OFF);
}

void LitimeBmsBle::set_discharging_enabled(bool enabled) {
  ESP_LOGI(TAG, "Setting discharging %s", enabled ? "ON" : "OFF");
  this->send_command_(enabled ? CMD_DISCHARGE_ON : CMD_DISCHARGE_OFF);
}

void LitimeBmsBle::publish_state_(sensor::Sensor *sensor, float value) {
  if (sensor != nullptr)
    sensor->publish_state(value);
}

void LitimeBmsBle::publish_state_(binary_sensor::BinarySensor *sensor, bool value) {
  if (sensor != nullptr)
    sensor->publish_state(value);
}

void LitimeBmsBle::publish_state_(text_sensor::TextSensor *sensor,
                                   const std::string &value) {
  if (sensor != nullptr)
    sensor->publish_state(value);
}

void LitimeBmsBle::publish_offline_() {
  this->publish_state_(this->online_binary_sensor_, false);
  this->publish_state_(this->total_voltage_sensor_, NAN);
  this->publish_state_(this->current_sensor_, NAN);
  this->publish_state_(this->power_sensor_, NAN);
  this->publish_state_(this->state_of_charge_sensor_, NAN);
  this->publish_state_(this->state_of_health_sensor_, NAN);
  this->publish_state_(this->cell_temperature_sensor_, NAN);
  this->publish_state_(this->mosfet_temperature_sensor_, NAN);
  this->publish_state_(this->remaining_capacity_sensor_, NAN);
  this->publish_state_(this->full_charge_capacity_sensor_, NAN);
  this->publish_state_(this->discharge_cycles_sensor_, NAN);
  this->publish_state_(this->total_discharge_ah_sensor_, NAN);
  this->publish_state_(this->min_cell_voltage_sensor_, NAN);
  this->publish_state_(this->max_cell_voltage_sensor_, NAN);
  this->publish_state_(this->delta_cell_voltage_sensor_, NAN);
  for (uint8_t i = 0; i < MAX_CELLS; i++) {
    this->publish_state_(this->cell_voltage_sensors_[i], NAN);
  }
}

// ============================================================================
// Switch dump_config
// ============================================================================

void LitimeChargingSwitch::dump_config() {
  ESP_LOGCONFIG(TAG, "LiTime Charging Switch");
}

void LitimeDischargingSwitch::dump_config() {
  ESP_LOGCONFIG(TAG, "LiTime Discharging Switch");
}

// ============================================================================
// LitimeBleScanner — passive discovery
// ============================================================================

void LitimeBleScanner::dump_config() {
  ESP_LOGCONFIG(TAG, "LiTime BLE Scanner:");
  ESP_LOGCONFIG(TAG, "  Scanning for devices with name prefix 'LT-'");
}

bool LitimeBleScanner::parse_device(const espbt::ESPBTDevice &device) {
  // Check if device name starts with "LT-"
  const std::string &name = device.get_name();
  if (name.empty() || name.substr(0, 3) != "LT-")
    return false;

  // Convert MAC to uint64 for deduplication
  auto addr = device.address_uint64();

  if (this->discovered_devices_.count(addr) > 0)
    return false;  // Already discovered, don't log again

  this->discovered_devices_.insert(addr);

  // Format MAC address
  char mac_str[18];
  const uint8_t *raw = device.address();
  snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
           raw[0], raw[1], raw[2], raw[3], raw[4], raw[5]);

  ESP_LOGI(TAG, "========================================");
  ESP_LOGI(TAG, "  Found LiTime device!");
  ESP_LOGI(TAG, "  Name: \"%s\"", name.c_str());
  ESP_LOGI(TAG, "  MAC:  %s", mac_str);
  ESP_LOGI(TAG, "  RSSI: %d dBm", device.get_rssi());
  ESP_LOGI(TAG, "  Add to your YAML:");
  ESP_LOGI(TAG, "    ble_client:");
  ESP_LOGI(TAG, "      - mac_address: \"%s\"", mac_str);
  ESP_LOGI(TAG, "========================================");

  return false;  // Don't claim the device, let other listeners see it too
}

}  // namespace litime_bms_ble
}  // namespace esphome

#endif  // USE_ESP32
