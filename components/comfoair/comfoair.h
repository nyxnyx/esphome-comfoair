#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/climate/climate_mode.h"
#include "esphome/components/climate/climate_traits.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/preferences.h"
#include "messages.h"

namespace esphome {
namespace comfoair {
class ComfoAirComponent;

struct ComfoAirSavedSettings {
  uint8_t preheater;
  uint8_t bypass;
  uint8_t type;
  uint8_t size;
  uint8_t options;
  uint8_t ex_levels[4];
  uint8_t su_levels[4];
  uint8_t delay_bath_on;
  uint8_t delay_bath_off;
  uint8_t delay_boost;
  uint8_t delay_filter_weeks;
  uint8_t initialized;
};

class ComfoAirAutoBalanceSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(ComfoAirComponent *parent) { parent_ = parent; }
  void setup() override {
    bool initial_state = this->get_initial_state().value_or(false);
    this->publish_state(initial_state);
  }
  void write_state(bool state) override;
 protected:
  ComfoAirComponent *parent_{nullptr};
};

class ComfoAirComponent : public climate::Climate, public PollingComponent, public uart::UARTDevice {
  friend class ComfoAirAutoBalanceSwitch;
public:

  // Poll every 600ms
  ComfoAirComponent() : 
  Climate(), 
  PollingComponent(600),
  UARTDevice() { }

  void setup() override {
    this->pref_ = global_preferences->make_preference<ComfoAirSavedSettings>(2849182947UL, true);
    ComfoAirSavedSettings saved{};
    if (this->pref_.load(&saved) && saved.initialized == 0xAA) {
      this->cached_preheater_ = saved.preheater;
      this->cached_bypass_ = saved.bypass;
      this->cached_type_ = saved.type;
      this->cached_size_ = saved.size;
      this->cached_options_ = saved.options;
      memcpy(this->cached_ex_levels_, saved.ex_levels, sizeof(this->cached_ex_levels_));
      memcpy(this->cached_su_levels_, saved.su_levels, sizeof(this->cached_su_levels_));
      this->cached_delay_bath_on_ = saved.delay_bath_on;
      this->cached_delay_bath_off_ = saved.delay_bath_off;
      this->cached_delay_boost_ = saved.delay_boost;
      this->cached_delay_filter_weeks_ = saved.delay_filter_weeks;
      this->hw_config_cached_ = true;
      this->ventilation_levels_cached_ = true;
      this->delays_cached_ = true;
      ESP_LOGI(TAG, "Restored ComfoAir hardware settings from ESP32 NVS Flash: Type=%u [1=Left], Size=%u [1=Large CA350], Bypass=%u, Preheater=%u, Options=%u",
               this->cached_type_, this->cached_size_, this->cached_bypass_, this->cached_preheater_, this->cached_options_);
    } else {
      ESP_LOGI(TAG, "No saved ComfoAir settings in ESP32 flash yet; will persist once queried from central unit.");
    }
  }

  /// Return the traits of this controller.
  climate::ClimateTraits traits() override {
    auto traits = climate::ClimateTraits();
    traits.set_supported_modes({climate::CLIMATE_MODE_FAN_ONLY, climate::CLIMATE_MODE_OFF});
    traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
    traits.set_visual_min_temperature(12);
    traits.set_visual_max_temperature(29);
    traits.set_visual_target_temperature_step(1.0);
    traits.set_supported_fan_modes({
      climate::CLIMATE_FAN_FOCUS,
      climate::CLIMATE_FAN_AUTO,
      climate::CLIMATE_FAN_LOW,
      climate::CLIMATE_FAN_MEDIUM,
      climate::CLIMATE_FAN_HIGH,
      climate::CLIMATE_FAN_OFF,
    }); 
    return traits;
  }

  /// Override control to change settings of the climate device.
  void control(const climate::ClimateCall &call) override {
    if (call.get_fan_mode().has_value()) {
      int level;

      this->fan_mode = *call.get_fan_mode();
      switch (this->fan_mode.value()) {
        case climate::CLIMATE_FAN_FOCUS:
          level = 0x05;
          break;

        case climate::CLIMATE_FAN_HIGH:
          level = 0x04;
          break;
        case climate::CLIMATE_FAN_MEDIUM:
          level = 0x03;
          break;
        case climate::CLIMATE_FAN_LOW:
          level = 0x02;
          break;
        case climate::CLIMATE_FAN_OFF:
          level = 0x01;
          break;
        case climate::CLIMATE_FAN_AUTO:
          level = 0x00;
          break;
        case climate::CLIMATE_FAN_ON:
        case climate::CLIMATE_FAN_MIDDLE:
        case climate::CLIMATE_FAN_DIFFUSE:
        default:
          level = -1;
          break;
      }

      if (level >= 0) {
        set_level_(level);
      }

    }
    if (call.get_target_temperature().has_value()) {
      this->target_temperature = *call.get_target_temperature();
      set_comfort_temperature_(this->target_temperature);
    }

    this->publish_state();
  }

  void dump_config() override {
    uint8_t *p;
    ESP_LOGCONFIG(TAG, "ComfoAir:");
    //LOG_UPDATE_INTERVAL(this);
    p = bootloader_version_;
    ESP_LOGCONFIG(TAG, "  Bootloader %.10s v%0d.%02d b%2d", p + 3, *p, *(p + 1), *(p + 2));
    p = firmware_version_;
    ESP_LOGCONFIG(TAG, "  Firmware %.10s v%0d.%02d b%2d", p + 3, *p, *(p + 1), *(p + 2));
    p = connector_board_version_;
    ESP_LOGCONFIG(TAG, "  Connector Board %.10s v%0d.%02d", p + 2, *p, *(p + 1));

    if (*(p + 12) != 0) {
      ESP_LOGCONFIG(TAG, "  CC-Ease v%0d.%02d", *(p + 12) >> 4, *(p + 12) & 0x0f);
    }
    if (*(p + 13) != 0) {
      ESP_LOGCONFIG(TAG, "  CC-Luxe v%0d.%02d", *(p + 13) >> 4, *(p + 13) & 0x0f);
    }
    this->check_uart_settings(9600);
  }

  void update()  {
    switch(update_counter_) {
      case -4:
        this->write_command_(COMFOAIR_GET_BOOTLOADER_VERSION_REQUEST, nullptr, 0);
        break;
      case -3:
        this->write_command_(COMFOAIR_GET_FIRMWARE_VERSION_REQUEST, nullptr, 0);
        break;
      case -2:
        this->write_command_(COMFOAIR_GET_BOARD_VERSION_REQUEST, nullptr, 0);
        break;
      case -1:
        get_main_status_();
        break;
      case 0:
        get_fan_status_();
        break;
      case 1:
        get_valve_status_();
        break;
      case 2:
        get_ventilation_level_();
        break;
      case 3:
        get_temperatures_();
        break;
      case 4:
        get_temperature_();
        break;
      case 5:
        get_error_status_();
        break;
      case 6:
        get_bypass_control_status_();
        break;
      case 7:
        get_delays_();
        break;
      case 8:
        get_analog_inputs_();
        break;
      case 9:
        get_main_status_();
        break;
    }

    update_counter_++;
    if (update_counter_ > 9)
      update_counter_ = 0;
  }

  void loop() override {
    while (this->available() != 0) {
      uint8_t byte;
      this->read_byte(&byte);

      if (this->data_index_ == 0) {
        if (byte == COMFOAIR_MSG_PREFIX) {
          this->data_[this->data_index_++] = byte;
        }
        continue;
      }

      if (this->data_index_ == 1) {
        if (byte == COMFOAIR_MSG_ACK) {
          // ACK: 0x07 0xF3
          this->data_index_ = 0;
          this->in_escape_ = false;
          continue;
        } else if (byte == COMFOAIR_MSG_HEAD) {
          // Head: 0x07 0xF0
          this->data_[this->data_index_++] = byte;
          this->in_escape_ = false;
          continue;
        } else if (byte == COMFOAIR_MSG_PREFIX) {
          // Stray prefix, keep data_index_ = 1
          this->data_[0] = COMFOAIR_MSG_PREFIX;
          this->data_index_ = 1;
          this->in_escape_ = false;
          continue;
        } else {
          this->data_index_ = 0;
          this->in_escape_ = false;
          continue;
        }
      }

      if (this->data_index_ < COMFOAIR_MSG_HEAD_LENGTH) {
        if (this->data_index_ == 2 && byte != 0x00) {
          this->data_index_ = 0;
          continue;
        }
        this->data_[this->data_index_++] = byte;
        continue;
      }

      // Payload / tail handling (data_index_ >= COMFOAIR_MSG_HEAD_LENGTH)
      if (this->in_escape_) {
        this->in_escape_ = false;
        if (byte == COMFOAIR_MSG_PREFIX) {
          // Escaped 0x07: "0x07 0x07" -> unescape to single 0x07
          if (this->data_index_ < sizeof(this->data_)) {
            this->data_[this->data_index_++] = COMFOAIR_MSG_PREFIX;
          }
        } else if (byte == COMFOAIR_MSG_TAIL) {
          // End of packet: "0x07 0x0F"
          uint8_t data_length = this->data_[COMFOAIR_MSG_DATA_LENGTH_IDX];
          uint8_t expected_total = COMFOAIR_MSG_HEAD_LENGTH + data_length + 1; // head(5) + data(N) + checksum(1)
          if (this->data_index_ == expected_total) {
            uint8_t rx_checksum = this->data_[expected_total - 1];
            uint8_t calc_checksum = comfoair_checksum_(this->data_ + 2, COMFOAIR_MSG_HEAD_LENGTH + data_length - 2);
            if (rx_checksum == calc_checksum) {
              this->parse_data_();
            } else {
              ESP_LOGW(TAG, "ComfoAir Checksum doesn't match: rx=0x%02X calc=0x%02X", rx_checksum, calc_checksum);
              ESP_LOGW(TAG, "Frame: %s", format_hex_pretty(this->data_, this->data_index_).c_str());
            }
          } else {
            ESP_LOGW(TAG, "ComfoAir Frame length mismatch: got %u, expected %u", this->data_index_, expected_total);
          }
          this->data_index_ = 0;
          continue;
        } else {
          ESP_LOGW(TAG, "Unexpected byte after 0x07: 0x%02X", byte);
          this->data_index_ = 0;
          continue;
        }
      } else {
        if (byte == COMFOAIR_MSG_PREFIX) {
          this->in_escape_ = true;
          continue;
        }
        if (this->data_index_ < sizeof(this->data_)) {
          this->data_[this->data_index_++] = byte;
        } else {
          ESP_LOGW(TAG, "ComfoAir RX buffer overflow, resetting");
          this->data_index_ = 0;
        }
      }
    }
    this->run_auto_balance_();
  }

  float get_setup_priority() const override { return setup_priority::DATA; }

  void reset_filter(void) {
    uint8_t reset_cmd[COMFOAIR_SET_RESET_LENGTH] = {0, 0, 0, 1};
	  ESP_LOGI(TAG, "Resetting filter counter. Make sure that filter is changed");
    this->write_command_(COMFOAIR_SET_RESET_REQUEST, reset_cmd, sizeof(reset_cmd));
	}

  void set_ventilation_levels(uint8_t ex_absent, uint8_t ex_low, uint8_t ex_medium, uint8_t ex_high,
                              uint8_t su_absent, uint8_t su_low, uint8_t su_medium, uint8_t su_high,
                              uint8_t current_speed = 0) {
    if (current_speed == 0) {
      current_speed = 2; // Default low (Bieg 1)
      if (this->fan_mode.has_value()) {
        auto fm = this->fan_mode.value();
        if (fm == climate::CLIMATE_FAN_OFF) current_speed = 1;
        else if (fm == climate::CLIMATE_FAN_LOW) current_speed = 2;
        else if (fm == climate::CLIMATE_FAN_MEDIUM) current_speed = 3;
        else if (fm == climate::CLIMATE_FAN_HIGH) current_speed = 4;
      }
    }
    uint8_t payload[COMFOAIR_SET_VENTILATION_LEVEL_LENGTH] = {
      ex_absent, ex_low, ex_medium,
      su_absent, su_low, su_medium,
      ex_high, su_high,
      current_speed
    };
    ESP_LOGI(TAG, "Setting ventilation levels: Ex=[%u,%u,%u,%u], Su=[%u,%u,%u,%u], Spd=%u",
             ex_absent, ex_low, ex_medium, ex_high,
             su_absent, su_low, su_medium, su_high, current_speed);
    this->write_command_(COMFOAIR_SET_VENTILATION_LEVEL_REQUEST, payload, sizeof(payload));

    if (current_speed > 0) {
      this->set_level_(current_speed);
      switch (current_speed) {
        case 1: this->fan_mode = climate::CLIMATE_FAN_OFF; break;
        case 2: this->fan_mode = climate::CLIMATE_FAN_LOW; break;
        case 3: this->fan_mode = climate::CLIMATE_FAN_MEDIUM; break;
        case 4: this->fan_mode = climate::CLIMATE_FAN_HIGH; break;
      }
      this->publish_state();
    }
  }

  void set_delays(uint8_t bath_on, uint8_t bath_off, uint8_t boost, uint8_t filter_weeks) {
    uint8_t payload[COMFOAIR_SET_DELAYS_LENGTH] = {
      bath_on, bath_off, boost, filter_weeks, 0, 0, 0, 0
    };
    ESP_LOGI(TAG, "Setting delays: BathOn=%u min, BathOff=%u min, Boost=%u min, Filter=%u wks",
             bath_on, bath_off, boost, filter_weeks);
    this->write_command_(COMFOAIR_SET_DELAYS_REQUEST, payload, sizeof(payload));
  }

  void set_rs232_mode(uint8_t mode) {
    uint8_t payload[COMFOAIR_SET_RS232_MODE_LENGTH] = {mode};
    ESP_LOGI(TAG, "Setting RS232 mode to %u (1=PC Master, 0=Normal)", mode);
    this->write_command_(COMFOAIR_SET_RS232_MODE_REQUEST, payload, sizeof(payload));
  }

  void restore_hardware_configuration(uint8_t type = 0xFF, uint8_t size = 0xFF, uint8_t options = 0xFF, uint8_t preheater = 0xFF, uint8_t bypass = 0xFF) {
    if (!this->hw_config_cached_ && type == 0xFF) {
      ESP_LOGW(TAG, "Cannot restore hardware configuration: no cached hardware parameters from central unit yet.");
      return;
    }
    uint8_t t = (type != 0xFF) ? type : this->cached_type_;
    uint8_t s = (size != 0xFF) ? size : this->cached_size_;
    uint8_t o = (options != 0xFF) ? options : this->cached_options_;
    uint8_t p = (preheater != 0xFF) ? preheater : this->cached_preheater_;
    uint8_t b = (bypass != 0xFF) ? bypass : this->cached_bypass_;

    uint8_t payload[COMFOAIR_SET_MAIN_STATUS_LENGTH] = {p, b, t, s, o, 0, 0, 0};
    ESP_LOGI(TAG, "Writing ComfoAir Hardware Config from cache (0xD7: Preheater=%u, Bypass=%u, Typ=%u, Size=%u, Options=%u)...", p, b, t, s, o);
    this->write_command_(COMFOAIR_SET_MAIN_STATUS_REQUEST, payload, sizeof(payload));

    this->cached_type_ = t;
    this->cached_size_ = s;
    this->cached_options_ = o;
    this->cached_preheater_ = p;
    this->cached_bypass_ = b;
    this->hw_config_cached_ = true;
    this->save_settings_to_flash_();
  }

  void query_versions() {
    ESP_LOGI(TAG, "Querying Firmware & Board Versions (0x69, 0xA1)...");
    this->write_command_(COMFOAIR_GET_FIRMWARE_VERSION_REQUEST, nullptr, 0);
    this->write_command_(COMFOAIR_GET_BOARD_VERSION_REQUEST, nullptr, 0);
  }

  void reset_faults() {
    // 0xDB resets: Byte 0=faults/errors, Byte 1=settings, Byte 2=self-test, Byte 3=filter
    uint8_t reset_cmd[COMFOAIR_SET_RESET_LENGTH] = {1, 0, 0, 0};
    ESP_LOGI(TAG, "Resetting faults and errors on ComfoAir 350 (0xDB)...");
    this->write_command_(COMFOAIR_SET_RESET_REQUEST, reset_cmd, sizeof(reset_cmd));
  }

  void factory_reset() {
    uint8_t reset_cmd[COMFOAIR_SET_RESET_LENGTH] = {0, 1, 0, 0};
    ESP_LOGI(TAG, "Factory Resetting ComfoAir 350 Settings (0xDB: [0,1,0,0])...");
    this->write_command_(COMFOAIR_SET_RESET_REQUEST, reset_cmd, sizeof(reset_cmd));
  }

  void start_self_test() {
    uint8_t reset_cmd[COMFOAIR_SET_RESET_LENGTH] = {0, 0, 1, 0};
    ESP_LOGI(TAG, "Starting ComfoAir 350 Self-Test (0xDB: [0,0,1,0])...");
    this->write_command_(COMFOAIR_SET_RESET_REQUEST, reset_cmd, sizeof(reset_cmd));
  }

  void end_test_mode() {
    uint8_t reset_cmd[COMFOAIR_SET_RESET_LENGTH] = {0, 0, 0, 0};
    ESP_LOGI(TAG, "Ending ComfoAir Test Mode & Clearing Self-Test (0xDB [0,0,0,0] & 0x19)...");
    this->write_command_(COMFOAIR_SET_RESET_REQUEST, reset_cmd, sizeof(reset_cmd));
    this->write_command_(COMFOAIR_SET_TEST_MODE_END_REQUEST, nullptr, 0);
  }

  void full_factory_reset_and_restore_hardware() {
    if (!this->hw_config_cached_) {
      ESP_LOGW(TAG, "Cannot reset and restore: hardware configuration not yet cached from central unit.");
      return;
    }
    ESP_LOGI(TAG, "=== WYKONUJE PELNY RESET FABRYCZNY I PRZYWRACANIE ZACHOWANEJ KONFIGURACJI Z CACHE ===");
    // 1. Reset parametrow EEPROM do fabrycznych (bledy=1, ustawienia=1, autotest=0, filtr=1)
    uint8_t reset_cmd[COMFOAIR_SET_RESET_LENGTH] = {1, 1, 0, 1};
    ESP_LOGI(TAG, "1. Resetowanie parametrow do fabrycznych (0xDB [1,1,0,1])...");
    this->write_command_(COMFOAIR_SET_RESET_REQUEST, reset_cmd, sizeof(reset_cmd));

    // 2. Wyjscie z trybu testowego
    ESP_LOGI(TAG, "2. Wyjscie z trybu testowego (0x19)...");
    this->write_command_(COMFOAIR_SET_TEST_MODE_END_REQUEST, nullptr, 0);

    // 3. Zapis konfiguracji sprzetowej dokladnie z cache
    uint8_t hw_payload[COMFOAIR_SET_MAIN_STATUS_LENGTH] = {
      this->cached_preheater_, this->cached_bypass_, this->cached_type_,
      this->cached_size_, this->cached_options_, 0, 0, 0
    };
    ESP_LOGI(TAG, "3. Zapis konfiguracji sprzetowej z cache (0xD7: Preheater=%u, Bypass=%u, Typ=%u, Size=%u, Options=%u)...",
             this->cached_preheater_, this->cached_bypass_, this->cached_type_, this->cached_size_, this->cached_options_);
    this->write_command_(COMFOAIR_SET_MAIN_STATUS_REQUEST, hw_payload, sizeof(hw_payload));

    // 4. Ustawienie trybu RS232 na 0 (Normalny / autonomiczny - przelacznik biegow Kincony)
    uint8_t mode_payload[COMFOAIR_SET_RS232_MODE_LENGTH] = {0};
    ESP_LOGI(TAG, "4. Przywrocenie trybu RS232 na 0 (Normalny / sterowanie przelacznikiem)...");
    this->write_command_(COMFOAIR_SET_RS232_MODE_REQUEST, mode_payload, sizeof(mode_payload));
  }

  void query_main_status() {
    ESP_LOGI(TAG, "Querying Main Status (0xD5)...");
    this->write_command_(COMFOAIR_GET_MAIN_STATUS_REQUEST, nullptr, 0);
  }

  void set_fan_level(uint8_t level) {
    this->set_level_(level);
  }

  void set_comfort_temperature(float temperature) {
    this->set_comfort_temperature_(temperature);
  }

  void set_bypass_mode(uint8_t mode) {
    // Zehnder CA350 bypass control via comfort temperature setpoint:
    // 0 = Auto: default comfort setpoint (21.0 C)
    // 1 = Force Close: comfort setpoint 29.0 C (heat retention - keeps bypass strictly closed)
    // 2 = Force Open: comfort setpoint 12.0 C (cooling - opens bypass)
    if (mode == 1) {
      ESP_LOGI(TAG, "Bypass mode: Force Close (setting comfort temp to 29.0 C)");
      this->set_comfort_temperature_(29.0f);
    } else if (mode == 2) {
      ESP_LOGI(TAG, "Bypass mode: Force Open (setting comfort temp to 12.0 C)");
      this->set_comfort_temperature_(12.0f);
    } else {
      ESP_LOGI(TAG, "Bypass mode: Auto (setting comfort temp to 21.0 C)");
      this->set_comfort_temperature_(21.0f);
    }
  }

  void set_ewt(uint8_t ewt_low, uint8_t ewt_high, uint8_t reheater_temp = 0) {
    uint8_t payload[COMFOAIR_SET_EWT_REHEATER_LENGTH] = {
      1, ewt_low, ewt_high, reheater_temp, 0
    };
    ESP_LOGI(TAG, "Setting EWT/Reheater: Low=%u C, High=%u C, Reheater=%u C", ewt_low, ewt_high, reheater_temp);
    this->write_command_(COMFOAIR_SET_EWT_REHEATER_REQUEST, payload, sizeof(payload));
  }

  void set_name(const char* value) {}
  void set_uart_component(uart::UARTComponent *parent) {this->set_uart_parent(parent);}

protected:

  void set_level_(int level) {
    if (level < 0 || level > 5) {
      ESP_LOGI(TAG, "Ignoring invalid level request: %i", level);
      return;
    }

    ESP_LOGI(TAG, "Setting level to: %i", level);
    {
      uint8_t command[COMFOAIR_SET_LEVEL_LENGTH] = {(uint8_t) level};
      this->write_command_(COMFOAIR_SET_LEVEL_REQUEST, command, sizeof(command));
    }
  }

  void set_comfort_temperature_(float temperature) {
    if (temperature < 12.0f || temperature > 29.0f) {
      ESP_LOGI(TAG, "Ignoring invalid temperature request: %.1f", temperature);
      return;
    }

    ESP_LOGI(TAG, "Setting temperature to: %.1f", temperature);
    {
      uint8_t command[COMFOAIR_SET_COMFORT_TEMPERATURE_LENGTH] = {(uint8_t) ((temperature + 20.0f) * 2.0f)};
      this->write_command_(COMFOAIR_SET_COMFORT_TEMPERATURE_REQUEST, command, sizeof(command));
    }
  }

  void write_escaped_byte_(uint8_t byte) {
    this->write_byte(byte);
    if (byte == COMFOAIR_MSG_PREFIX) {
      this->write_byte(COMFOAIR_MSG_PREFIX);
    }
  }

  void write_command_(const uint8_t command, const uint8_t *command_data, uint8_t command_data_length) {
    this->write_byte(COMFOAIR_MSG_PREFIX);
    this->write_byte(COMFOAIR_MSG_HEAD);
    this->write_escaped_byte_(0x00);
    this->write_escaped_byte_(command);
    this->write_escaped_byte_(command_data_length);
    if (command_data_length > 0) {
      for (uint8_t i = 0; i < command_data_length; i++) {
        this->write_escaped_byte_(command_data[i]);
      }
      uint8_t checksum = (command + command_data_length + comfoair_checksum_(command_data, command_data_length)) & 0xff;
      this->write_escaped_byte_(checksum);
    } else {
      uint8_t checksum = comfoair_checksum_(&command, 1);
      this->write_escaped_byte_(checksum);
    }
    this->write_byte(COMFOAIR_MSG_PREFIX);
    this->write_byte(COMFOAIR_MSG_TAIL);
    this->flush();
  }

  uint8_t comfoair_checksum_(const uint8_t *command_data, uint8_t length) const {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < length; i++) {
      sum += command_data[i];
    }
    return sum + 0xad;
  }

  optional<bool> check_byte_() const {
    uint8_t index = this->data_index_;
    uint8_t byte = this->data_[index];

    if (index == 0) {
      return byte == COMFOAIR_MSG_PREFIX;
    }

    if (index == 1) {
      if (byte == COMFOAIR_MSG_ACK) {
        return {};
      } else {
        return byte == COMFOAIR_MSG_HEAD;
      }
    }

    if (index == 2) {
      return byte == 0x00;
    }

    if (index < COMFOAIR_MSG_HEAD_LENGTH) {
      return true;
    }

    uint8_t data_length = this->data_[COMFOAIR_MSG_DATA_LENGTH_IDX];

    if ((COMFOAIR_MSG_HEAD_LENGTH + data_length + COMFOAIR_MSG_TAIL_LENGTH) > sizeof(this->data_)) {
      ESP_LOGW(TAG, "ComfoAir message too large");
      return false;
    }

    if (index < COMFOAIR_MSG_HEAD_LENGTH + data_length) {
      return true;
    }

    if (index == COMFOAIR_MSG_HEAD_LENGTH + data_length) {
      // checksum is without checksum bytes
      uint8_t checksum = comfoair_checksum_(this->data_ + 2, COMFOAIR_MSG_HEAD_LENGTH + data_length - 2);
      if (checksum != byte) {
        //ESP_LOGW(TAG, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", this->data_[0], this->data_[1], this->data_[2], this->data_[3], this->data_[4], this->data_[5], this->data_[6], this->data_[7], this->data_[8], this->data_[9], this->data_[10]);
        ESP_LOGW(TAG, "ComfoAir Checksum doesn't match: 0x%02X!=0x%02X", byte, checksum);
        return false;
      }
      return true;
    }

    if (index == COMFOAIR_MSG_HEAD_LENGTH + data_length + 1) {
      return byte == COMFOAIR_MSG_PREFIX;
    }

    if (index == COMFOAIR_MSG_HEAD_LENGTH + data_length + 2) {
      if (byte != COMFOAIR_MSG_TAIL) {
        return false;
      }
    }

    return {};
  }

  void publish_temperatures_(float t1, float t2, float t3, float t4) {
    // Zehnder ComfoAir internal NTC channels:
    // Right-hand unit (Type 0 / 2):
    //   T1 = Outside Air (intake)
    //   T2 = Supply Air (fresh air into house)
    //   T3 = Return Air (stale air from rooms)
    //   T4 = Exhaust Air (stale air to outside)
    // Left-hand unit (Type 1):
    //   Physical duct paths and fan scrolls are mirrored.
    //   T1 = Supply Air (fresh air into house)
    //   T2 = Exhaust Air (stale air to outside)
    //   T3 = Outside Air (fresh intake from outside)
    //   T4 = Return Air (stale air from rooms)
    float outside = t1;
    float supply = t2;
    float return_air = t3;
    float exhaust = t4;

    if (this->cached_type_ == 1) {
      supply = t1;
      exhaust = t2;
      outside = t3;
      return_air = t4;
    }

    if (this->outside_air_temperature != nullptr) this->outside_air_temperature->publish_state(outside);
    if (this->supply_air_temperature != nullptr) this->supply_air_temperature->publish_state(supply);
    if (this->return_air_temperature != nullptr) this->return_air_temperature->publish_state(return_air);
    if (this->exhaust_air_temperature != nullptr) this->exhaust_air_temperature->publish_state(exhaust);
  }

  void parse_data_() {
    this->status_clear_warning();
    uint8_t *msg = &this->data_[COMFOAIR_MSG_HEAD_LENGTH];

    switch (this->data_[COMFOAIR_MSG_IDENTIFIER_IDX]) {
      case COMFOAIR_GET_BOOTLOADER_VERSION_RESPONSE: {
        size_t len = std::min<size_t>(sizeof(this->bootloader_version_), this->data_[COMFOAIR_MSG_DATA_LENGTH_IDX]);
        memcpy(this->bootloader_version_, msg, len);
        break;
      }
      case COMFOAIR_GET_FIRMWARE_VERSION_RESPONSE: {
        size_t len = std::min<size_t>(sizeof(this->firmware_version_), this->data_[COMFOAIR_MSG_DATA_LENGTH_IDX]);
        memcpy(this->firmware_version_, msg, len);
        ESP_LOGI(TAG, "0x6A Firmware Version: %.*s (len=%u)", (int)len, msg, (unsigned)len);
        break;
      }
      case COMFOAIR_GET_BOARD_VERSION_RESPONSE: {
        size_t len = std::min<size_t>(sizeof(this->connector_board_version_), this->data_[COMFOAIR_MSG_DATA_LENGTH_IDX]);
        memcpy(this->connector_board_version_, msg, len);
        ESP_LOGI(TAG, "0xA2 Board Version: %.*s (len=%u)", (int)len, msg, (unsigned)len);
        break;
      }
      case COMFOAIR_GET_FAN_STATUS_RESPONSE: {
          uint16_t raw_su = this->get_uint16_(2);
          uint16_t raw_ex = this->get_uint16_(4);
          ESP_LOGI(TAG, "0x0C Fan Status: Supply=%u%% (raw=%u), Exhaust=%u%% (raw=%u)", msg[0], raw_su, msg[1], raw_ex);
          if (this->fan_supply_percentage != nullptr) {
            this->fan_supply_percentage->publish_state(msg[0]);
          }
          if (this->fan_exhaust_percentage != nullptr) {
            this->fan_exhaust_percentage->publish_state(msg[1]);
          }
          if (this->supply_air_level != nullptr) {
            this->supply_air_level->publish_state(msg[0]);
          }
          if (this->return_air_level != nullptr) {
            this->return_air_level->publish_state(msg[1]);
          }
          if (this->fan_speed_supply != nullptr) {
            uint16_t raw_su = this->get_uint16_(2);
            this->fan_speed_supply->publish_state((raw_su > 0 && raw_su != 0xFFFF) ? (1875000.0f / raw_su) : 0.0f);
          }
          if (this->fan_speed_exhaust != nullptr) {
            uint16_t raw_ex = this->get_uint16_(4);
            this->fan_speed_exhaust->publish_state((raw_ex > 0 && raw_ex != 0xFFFF) ? (1875000.0f / raw_ex) : 0.0f);
          }
          break;
        }
      case COMFOAIR_GET_VALVE_STATUS_RESPONSE: {
        if (this->is_bypass_valve_open != nullptr) {
          this->is_bypass_valve_open->publish_state(msg[0] != 0);
        }
        if (this->is_preheating != nullptr) {
            this->is_preheating->publish_state(msg[1] != 0);
        }
        break;
      }
      case COMFOAIR_GET_BYPASS_CONTROL_RESPONSE: {
        if (this->bypass_factor != nullptr) {
          this->bypass_factor->publish_state(msg[2]);
        }
        if (this->bypass_step != nullptr) {
          this->bypass_step->publish_state(msg[3]);
        }
        if (this->bypass_correction != nullptr) {
          this->bypass_correction->publish_state(msg[4]);
        }
        if (this->is_summer_mode != nullptr) {
          this->is_summer_mode->publish_state(msg[6] != 0);
        }
        break;
      }
      case COMFOAIR_GET_TEMPERATURE_RESPONSE: {
        float t1 = (float) msg[0] / 2.0f - 20.0f;
        float t2 = (float) msg[1] / 2.0f - 20.0f;
        float t3 = (float) msg[2] / 2.0f - 20.0f;
        float t4 = (float) msg[3] / 2.0f - 20.0f;
        ESP_LOGI(TAG, "0x10 Physical Temps: T1=%.1f, T2=%.1f, T3=%.1f, T4=%.1f", t1, t2, t3, t4);
        this->publish_temperatures_(t1, t2, t3, t4);
        break;
      }
      case 0x1A: {
        ESP_LOGI(TAG, "0x1A: ComfoAir acknowledged Test Mode ended successfully");
        break;
      }
      case COMFOAIR_GET_MAIN_STATUS_RESPONSE: {
        ESP_LOGI(TAG, "0xD6 Main Status: Preheater=%u, Bypass=%u, Type=%u, Size=%u, Options=%u",
                 msg[0], msg[1], msg[2], msg[3], msg[4]);
        if (msg[2] != 0xFF && msg[3] != 0xFF) {
          this->cached_preheater_ = msg[0];
          this->cached_bypass_ = msg[1];
          this->cached_type_ = msg[2];
          this->cached_size_ = msg[3];
          this->cached_options_ = msg[4];
          this->hw_config_cached_ = true;
          this->save_settings_to_flash_();
          ESP_LOGD(TAG, "Hardware config cached in RAM & NVS: Preheater=%u, Bypass=%u, Typ=%u, Size=%u, Options=%u",
                   this->cached_preheater_, this->cached_bypass_, this->cached_type_, this->cached_size_, this->cached_options_);
        } else {
          ESP_LOGW(TAG, "ComfoAir returned unconfigured status (Type=0xFF, Size=0xFF). Preserving safe defaults in RAM.");
        }
        break;
      }
      case COMFOAIR_GET_SENSOR_DATA_RESPONSE: {

        if (this->enthalpy_temperature != nullptr) {
          this->enthalpy_temperature->publish_state((float) msg[0] / 2.0f - 20.0f);
        }

        break;
      }
      case COMFOAIR_GET_VENTILATION_LEVEL_RESPONSE: {

        ESP_LOGI(TAG, "0xCE Levels: Ex=[%u,%u,%u,%u], Su=[%u,%u,%u,%u], CurEx=%u%%, CurSu=%u%%, Lvl=%u",
                 msg[0], msg[1], msg[2], msg[10],
                 msg[3], msg[4], msg[5], msg[11],
                 msg[6], msg[7], msg[8]);

        // Cache programmed stage percentages in RAM and persist to flash
        this->cached_ex_levels_[0] = msg[0];
        this->cached_ex_levels_[1] = msg[1];
        this->cached_ex_levels_[2] = msg[2];
        this->cached_ex_levels_[3] = msg[10];
        this->cached_su_levels_[0] = msg[3];
        this->cached_su_levels_[1] = msg[4];
        this->cached_su_levels_[2] = msg[5];
        this->cached_su_levels_[3] = msg[11];
        this->ventilation_levels_cached_ = true;
        this->save_settings_to_flash_();

        if (this->return_air_level != nullptr) {
          this->return_air_level->publish_state(msg[6]);
        }
        if (this->supply_air_level != nullptr) {
          this->supply_air_level->publish_state(msg[7]);
        }

        // Programmed stage percentages (Absent, Low, Medium, High)
        if (this->fan_exhaust_absent != nullptr) {
          this->fan_exhaust_absent->publish_state(msg[0]);
        }
        if (this->fan_exhaust_low != nullptr) {
          this->fan_exhaust_low->publish_state(msg[1]);
        }
        if (this->fan_exhaust_medium != nullptr) {
          this->fan_exhaust_medium->publish_state(msg[2]);
        }
        if (this->fan_supply_absent != nullptr) {
          this->fan_supply_absent->publish_state(msg[3]);
        }
        if (this->fan_supply_low != nullptr) {
          this->fan_supply_low->publish_state(msg[4]);
        }
        if (this->fan_supply_medium != nullptr) {
          this->fan_supply_medium->publish_state(msg[5]);
        }
        if (this->fan_exhaust_high != nullptr) {
          this->fan_exhaust_high->publish_state(msg[10]);
        }
        if (this->fan_supply_high != nullptr) {
          this->fan_supply_high->publish_state(msg[11]);
        }

        // Fan Speed
        switch(msg[8]) {
          case 0x00:
            this->fan_mode = climate::CLIMATE_FAN_AUTO;
            this->mode = climate::CLIMATE_MODE_AUTO;
            break;
          case 0x01:
            this->fan_mode = climate::CLIMATE_FAN_OFF;
            this->mode = climate::CLIMATE_MODE_OFF;
            break;
          case 0x02:
            this->fan_mode = climate::CLIMATE_FAN_LOW;
            this->mode = climate::CLIMATE_MODE_FAN_ONLY;
            break;
          case 0x03:
            this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
            this->mode = climate::CLIMATE_MODE_FAN_ONLY;
          break;
          case 0x04:
            this->fan_mode = climate::CLIMATE_FAN_HIGH;
            this->mode = climate::CLIMATE_MODE_FAN_ONLY;
            break;
        }

        this->publish_state();

        // Supply air fan active (1 = active / 0 = inactive)
        if (this->is_supply_fan_active != nullptr) {
          this->is_supply_fan_active->publish_state(msg[9] == 1);
        }
        break;
      }
      case COMFOAIR_GET_OPERATING_HOURS_RESPONSE: {
        // Operating hours are 3-byte (24-bit) Big Endian values in message payload
        if (this->fan_hours_supply != nullptr) this->fan_hours_supply->publish_state(this->get_uint24_(0));
        if (this->fan_hours_exhaust != nullptr) this->fan_hours_exhaust->publish_state(this->get_uint24_(3));
        if (this->bypass_hours != nullptr) this->bypass_hours->publish_state(this->get_uint24_(6));
        if (this->preheater_hours != nullptr) this->preheater_hours->publish_state(this->get_uint24_(9));
        if (this->filter_hours != nullptr) this->filter_hours->publish_state(this->get_uint24_(12));
        if (this->ewt_hours != nullptr) this->ewt_hours->publish_state(this->get_uint24_(15));
        break;
      }
      case COMFOAIR_GET_PREHEATER_STATUS_RESPONSE: {
        if (this->is_preheating != nullptr) {
          this->is_preheating->publish_state(msg[0] != 0);
        }
        break;
      }
      case COMFOAIR_GET_ERROR_STATE_RESPONSE: {
        if (this->is_filter_full != nullptr) {
          this->is_filter_full->publish_state(msg[8] != 0);
        }
        if (this->error_code != nullptr) {
          this->error_code->publish_state(msg[0]); // Just taking the first error byte as code for now
        }
        // Bitwise fault diagnostics
        if (this->is_t1_sensor_fault != nullptr) this->is_t1_sensor_fault->publish_state((msg[5] & (1 << 0)) != 0);
        if (this->is_t2_sensor_fault != nullptr) this->is_t2_sensor_fault->publish_state((msg[5] & (1 << 1)) != 0);
        if (this->is_t3_sensor_fault != nullptr) this->is_t3_sensor_fault->publish_state((msg[5] & (1 << 2)) != 0);
        if (this->is_t4_sensor_fault != nullptr) this->is_t4_sensor_fault->publish_state((msg[5] & (1 << 3)) != 0);
        if (this->is_preheater_fault != nullptr) this->is_preheater_fault->publish_state((msg[5] & (1 << 4)) != 0);
        if (this->is_supply_fan_fault != nullptr) this->is_supply_fan_fault->publish_state((msg[6] & (1 << 0)) != 0);
        if (this->is_exhaust_fan_fault != nullptr) this->is_exhaust_fan_fault->publish_state((msg[6] & (1 << 1)) != 0);
        if (this->is_bypass_fault != nullptr) this->is_bypass_fault->publish_state((msg[6] & (1 << 2)) != 0);
        break;
      }
      case COMFOAIR_GET_DELAYS_RESPONSE: {
        this->cached_delay_bath_on_ = msg[0];
        this->cached_delay_bath_off_ = msg[1];
        this->cached_delay_boost_ = msg[2];
        this->cached_delay_filter_weeks_ = msg[3];
        this->delays_cached_ = true;
        this->save_settings_to_flash_();

        if (this->delay_bathroom_switch_on != nullptr) this->delay_bathroom_switch_on->publish_state(msg[0]);
        if (this->delay_bathroom_switch_off != nullptr) this->delay_bathroom_switch_off->publish_state(msg[1]);
        if (this->delay_boost != nullptr) this->delay_boost->publish_state(msg[2]);
        if (this->delay_filter_weeks != nullptr) this->delay_filter_weeks->publish_state(msg[3]);
        break;
      }
      case COMFOAIR_GET_ANALOG_INPUTS_RESPONSE: {
        if (this->analog_input_1 != nullptr) this->analog_input_1->publish_state(msg[0]);
        if (this->analog_input_2 != nullptr) this->analog_input_2->publish_state(msg[1]);
        if (this->analog_input_3 != nullptr) this->analog_input_3->publish_state(msg[2]);
        if (this->analog_input_4 != nullptr) this->analog_input_4->publish_state(msg[3]);
        break;
      }
      case COMFOAIR_GET_EWT_REHEATER_RESPONSE: {
        if (this->ewt_state != nullptr) this->ewt_state->publish_state(msg[0]);
        if (this->ewt_temperature_low != nullptr) this->ewt_temperature_low->publish_state((float) msg[1] / 2.0f - 20.0f);
        if (this->ewt_temperature_high != nullptr) this->ewt_temperature_high->publish_state((float) msg[2] / 2.0f - 20.0f);
        if (this->reheater_state != nullptr) this->reheater_state->publish_state(msg[3]);
        break;
      }
      case COMFOAIR_GET_TEMPERATURES_RESPONSE: {

        // comfort temperature
        this->target_temperature = (float) msg[0] / 2.0f - 20.0f;
        this->publish_state();

        float t1 = (float) msg[1] / 2.0f - 20.0f;
        float t2 = (float) msg[2] / 2.0f - 20.0f;
        float t3 = (float) msg[3] / 2.0f - 20.0f;
        float t4 = (float) msg[4] / 2.0f - 20.0f;

        ESP_LOGI(TAG, "0xD2 Temps: Comfort=%.1f C, Mask=0x%02X, T1=%.1f, T2=%.1f, T3=%.1f, T4=%.1f",
                 this->target_temperature, msg[5], t1, t2, t3, t4);

        if ((msg[5] & 0x0F) == 0x0F) {
          this->publish_temperatures_(t1, t2, t3, t4);
        }

        // EWT
        if (this->ewt_temperature != nullptr && msg[5] & 0x10) {
          this->ewt_temperature->publish_state((float) msg[6] / 2.0f - 20.0f);
        }
        // reheating
        if (this->reheating_temperature != nullptr && msg[5] & 0x20) {
          this->reheating_temperature->publish_state((float) msg[7] / 2.0f - 20.0f);
        }
        // kitchen hood
        if (this->kitchen_hood_temperature != nullptr && msg[5] & 0x40) {
          this->kitchen_hood_temperature->publish_state((float) msg[8] / 2.0f - 20.0f);
        }

        break;
      }
    }
  }

  void get_fan_status_() {
    if (this->fan_supply_percentage != nullptr ||
        this->fan_exhaust_percentage != nullptr ||
        this->fan_speed_supply != nullptr ||
        this->fan_speed_exhaust != nullptr) {
      ESP_LOGD(TAG, "getting fan status");
      this->write_command_(COMFOAIR_GET_FAN_STATUS_REQUEST, nullptr, 0);
    }
  }

  void get_valve_status_() {
    if (this->is_bypass_valve_open != nullptr ||
        this->is_preheating != nullptr) {
      ESP_LOGD(TAG, "getting valve status");
      this->write_command_(COMFOAIR_GET_VALVE_STATUS_REQUEST, nullptr, 0);
    }
  }

  void get_error_status_() {
    if (this->is_filter_full != nullptr || this->error_code != nullptr ||
        this->is_supply_fan_fault != nullptr || this->is_exhaust_fan_fault != nullptr ||
        this->is_t1_sensor_fault != nullptr || this->is_t2_sensor_fault != nullptr ||
        this->is_t3_sensor_fault != nullptr || this->is_t4_sensor_fault != nullptr ||
        this->is_bypass_fault != nullptr || this->is_preheater_fault != nullptr) {
      ESP_LOGD(TAG, "getting error status");
      this->write_command_(COMFOAIR_GET_ERROR_STATE_REQUEST, nullptr, 0);
    }
  }

  void get_bypass_control_status_() {
    if (this->bypass_factor != nullptr ||
      this->bypass_step != nullptr ||
      this->bypass_correction != nullptr ||
      this->is_summer_mode != nullptr) {
      ESP_LOGD(TAG, "getting bypass control");
      this->write_command_(COMFOAIR_GET_BYPASS_CONTROL_REQUEST, nullptr, 0);
    }
  }

  void get_temperature_() {
    if (this->outside_air_temperature != nullptr ||
      this->supply_air_temperature != nullptr ||
      this->return_air_temperature != nullptr ||
      this->outside_air_temperature != nullptr) {
      ESP_LOGD(TAG, "getting temperature");
      this->write_command_(COMFOAIR_GET_TEMPERATURE_REQUEST, nullptr, 0);
    }
  }

  void get_sensor_data_() {
    if (this->enthalpy_temperature != nullptr) {
      ESP_LOGD(TAG, "getting sensor data");
      this->write_command_(COMFOAIR_GET_SENSOR_DATA_REQUEST, nullptr, 0);
    }
  }

  void get_main_status_() {
    ESP_LOGD(TAG, "getting main status");
    this->write_command_(COMFOAIR_GET_MAIN_STATUS_REQUEST, nullptr, 0);
  }

  void get_ventilation_level_() {
    ESP_LOGD(TAG, "getting ventilation level");
    this->write_command_(COMFOAIR_GET_VENTILATION_LEVEL_REQUEST, nullptr, 0);
  }

  void get_temperatures_() {
    ESP_LOGD(TAG, "getting temperatures");
    this->write_command_(COMFOAIR_GET_TEMPERATURES_REQUEST, nullptr, 0);
  }

  void get_delays_() {
    if (this->delay_bathroom_switch_on != nullptr || this->delay_bathroom_switch_off != nullptr ||
        this->delay_boost != nullptr || this->delay_filter_weeks != nullptr) {
      ESP_LOGD(TAG, "getting delays");
      this->write_command_(COMFOAIR_GET_DELAYS_REQUEST, nullptr, 0);
    }
  }

  void get_analog_inputs_() {
    if (this->analog_input_1 != nullptr || this->analog_input_2 != nullptr ||
        this->analog_input_3 != nullptr || this->analog_input_4 != nullptr) {
      ESP_LOGD(TAG, "getting analog inputs");
      this->write_command_(COMFOAIR_GET_ANALOG_INPUTS_REQUEST, nullptr, 0);
    }
  }

  void get_ewt_reheater_status_() {
    if (this->ewt_state != nullptr || this->reheater_state != nullptr ||
        this->ewt_temperature_low != nullptr || this->ewt_temperature_high != nullptr) {
      ESP_LOGD(TAG, "getting EWT and reheater status");
      this->write_command_(COMFOAIR_GET_EWT_REHEATER_REQUEST, nullptr, 0);
    }
  }

  void run_auto_balance_(bool force = false) {
    if (this->auto_balance_switch_ == nullptr || !this->auto_balance_switch_->state) {
      return;
    }
    if (this->supply_air_temperature == nullptr || this->return_air_temperature == nullptr) {
      return;
    }

    // Only run every 5 minutes unless forced
    uint32_t now = millis();
    if (force || now - last_auto_balance_ > 300000) {
      last_auto_balance_ = now;
      
      float supply = this->supply_air_temperature->state;
      float return_air = this->return_air_temperature->state;
      float target = this->target_temperature;
      
      if (std::isnan(supply) || std::isnan(return_air) || std::isnan(target)) return;

      // Real-time thermal effect: positive = cooling the house, negative = heating the house
      float thermal_power = return_air - supply;
      // Thermal demand: positive = we want to cool down, negative = we want to heat up
      float demand = return_air - target;

      bool is_helping = false;
      // We consider the system is helping if the thermal power has the same sign as the demand
      // (e.g. we want to cool (+) and the supply air is cooler than return air (+))
      if (std::abs(demand) > 0.3f) {
          if ((demand > 0 && thermal_power > 0.5f) || (demand < 0 && thermal_power < -0.5f)) {
              is_helping = true;
          }
      }

      // Safety: if preheating is active, don't boost fans (avoid wasting energy)
      if (this->is_preheating != nullptr && this->is_preheating->state && is_helping) {
          ESP_LOGD(TAG, "Auto balance: System is helping but Pre-heating is ACTIVE. Throttling boost.");
          is_helping = false; 
      }

      int new_level = 2; // Default to Stage 1 (Standard)
      float abs_demand = std::abs(demand);

      if (is_helping) {
          // Proportional response
          if (abs_demand > 3.0f)      new_level = 4; // Stage 3 (Boost)
          else if (abs_demand > 1.2f) new_level = 3; // Stage 2 (High)
          else                        new_level = 2; // Stage 1 (Standard)
          
          ESP_LOGI(TAG, "Auto balance: HELPING (Power: %.1fC). Demand: %.1fC -> Level %d", 
                   thermal_power, demand, new_level);
      } else {
          // System is counter-productive or we are at target
          if (abs_demand > 1.0f) {
              new_level = 1; // Stage 0 (Absent) - Minimize unproductive air exchange
              ESP_LOGI(TAG, "Auto balance: COUNTER-PRODUCTIVE (Power: %.1fC). Reducing to Minimum (Level 1).", thermal_power);
          } else {
              new_level = 2; // Stage 1 (Standard) - Normal ventilation near target
          }
      }
      
      this->set_level_(new_level);
    }
  }

  void get_operating_hours_() {
    ESP_LOGD(TAG, "getting operating hours");
    this->write_command_(COMFOAIR_GET_OPERATING_HOURS_REQUEST, nullptr, 0);
  }

  void get_preheater_status_() {
    ESP_LOGD(TAG, "getting preheater status");
    this->write_command_(COMFOAIR_GET_PREHEATER_STATUS_REQUEST, nullptr, 0);
  }

  uint8_t get_uint8_t_(uint8_t start_index) const {
    return this->data_[COMFOAIR_MSG_HEAD_LENGTH + start_index];
  }

  uint32_t get_uint24_(uint8_t start_index) const {
    return (uint32_t(this->data_[COMFOAIR_MSG_HEAD_LENGTH + start_index]) << 16) |
           (uint32_t(this->data_[COMFOAIR_MSG_HEAD_LENGTH + start_index + 1]) << 8) |
           uint32_t(this->data_[COMFOAIR_MSG_HEAD_LENGTH + start_index + 2]);
  }

  uint16_t get_uint16_(uint8_t start_index) const {
    return (uint16_t(this->data_[COMFOAIR_MSG_HEAD_LENGTH + start_index]) << 8) |
           uint16_t(this->data_[COMFOAIR_MSG_HEAD_LENGTH + start_index + 1]);
  }

  uint8_t data_[64]{0};
  uint8_t data_index_{0};
  bool in_escape_{false};
  int8_t update_counter_{-4};
  uint32_t last_auto_balance_{0};

  // Live cached EEPROM configuration in RAM (fetched upon first connection & kept in sync)
  bool hw_config_cached_{false};
  uint8_t cached_preheater_{0};
  uint8_t cached_bypass_{1};
  uint8_t cached_type_{1};      // 1 = Left, 0/2 = Right
  uint8_t cached_size_{1};      // 1 = Large (CA350), 0 = Small
  uint8_t cached_options_{0};   // 0 = CC-Ease basic, 1 = Luxe

  bool ventilation_levels_cached_{false};
  uint8_t cached_ex_levels_[4]{15, 40, 70, 90};
  uint8_t cached_su_levels_[4]{15, 40, 70, 90};

  bool delays_cached_{false};
  uint8_t cached_delay_bath_on_{0};
  uint8_t cached_delay_bath_off_{30};
  uint8_t cached_delay_boost_{0};
  uint8_t cached_delay_filter_weeks_{30};

  uint8_t bootloader_version_[13]{0};
  uint8_t firmware_version_[13]{0};
  uint8_t connector_board_version_[14]{0};
  ESPPreferenceObject pref_;

  void save_settings_to_flash_() {
    ComfoAirSavedSettings to_save{};
    to_save.preheater = this->cached_preheater_;
    to_save.bypass = this->cached_bypass_;
    to_save.type = this->cached_type_;
    to_save.size = this->cached_size_;
    to_save.options = this->cached_options_;
    memcpy(to_save.ex_levels, this->cached_ex_levels_, sizeof(to_save.ex_levels));
    memcpy(to_save.su_levels, this->cached_su_levels_, sizeof(to_save.su_levels));
    to_save.delay_bath_on = this->cached_delay_bath_on_;
    to_save.delay_bath_off = this->cached_delay_bath_off_;
    to_save.delay_boost = this->cached_delay_boost_;
    to_save.delay_filter_weeks = this->cached_delay_filter_weeks_;
    to_save.initialized = 0xAA;
    this->pref_.save(&to_save);
    ESP_LOGD(TAG, "Saved ComfoAir configuration to ESP32 Flash/NVS.");
  }

public:
  uint8_t get_cached_type() const { return this->cached_type_; }
  uint8_t get_cached_size() const { return this->cached_size_; }
  uint8_t get_cached_preheater() const { return this->cached_preheater_; }
  uint8_t get_cached_bypass() const { return this->cached_bypass_; }
  uint8_t get_cached_options() const { return this->cached_options_; }
  bool is_hw_config_cached() const { return this->hw_config_cached_; }

public: 
  sensor::Sensor *fan_supply_percentage{nullptr};
  sensor::Sensor *fan_exhaust_percentage{nullptr};
  sensor::Sensor *fan_speed_supply{nullptr};
  sensor::Sensor *fan_speed_exhaust{nullptr};
  sensor::Sensor *outside_air_temperature{nullptr};
  sensor::Sensor *supply_air_temperature{nullptr};
  sensor::Sensor *return_air_temperature{nullptr};
  sensor::Sensor *exhaust_air_temperature{nullptr};
  sensor::Sensor *enthalpy_temperature{nullptr};
  sensor::Sensor *ewt_temperature{nullptr};
  sensor::Sensor *reheating_temperature{nullptr};
  sensor::Sensor *kitchen_hood_temperature{nullptr};
  sensor::Sensor *return_air_level{nullptr};
  sensor::Sensor *supply_air_level{nullptr};
  sensor::Sensor *bypass_factor{nullptr};
  sensor::Sensor *bypass_step{nullptr};
  sensor::Sensor *bypass_correction{nullptr};
  binary_sensor::BinarySensor *is_bypass_valve_open{nullptr};
  binary_sensor::BinarySensor *is_preheating{nullptr};
  binary_sensor::BinarySensor *is_summer_mode{nullptr};
  sensor::Sensor *fan_exhaust_absent{nullptr};
  sensor::Sensor *fan_exhaust_low{nullptr};
  sensor::Sensor *fan_exhaust_medium{nullptr};
  sensor::Sensor *fan_exhaust_high{nullptr};
  sensor::Sensor *fan_supply_absent{nullptr};
  sensor::Sensor *fan_supply_low{nullptr};
  sensor::Sensor *fan_supply_medium{nullptr};
  sensor::Sensor *fan_supply_high{nullptr};
  binary_sensor::BinarySensor *is_supply_fan_active{nullptr};
  binary_sensor::BinarySensor *is_filter_full{nullptr};
  sensor::Sensor *fan_hours_supply{nullptr};
  sensor::Sensor *fan_hours_exhaust{nullptr};
  sensor::Sensor *bypass_hours{nullptr};
  sensor::Sensor *preheater_hours{nullptr};
  sensor::Sensor *filter_hours{nullptr};
  sensor::Sensor *ewt_hours{nullptr};
  sensor::Sensor *error_code{nullptr};
  binary_sensor::BinarySensor *is_supply_fan_fault{nullptr};
  binary_sensor::BinarySensor *is_exhaust_fan_fault{nullptr};
  binary_sensor::BinarySensor *is_t1_sensor_fault{nullptr};
  binary_sensor::BinarySensor *is_t2_sensor_fault{nullptr};
  binary_sensor::BinarySensor *is_t3_sensor_fault{nullptr};
  binary_sensor::BinarySensor *is_t4_sensor_fault{nullptr};
  binary_sensor::BinarySensor *is_bypass_fault{nullptr};
  binary_sensor::BinarySensor *is_preheater_fault{nullptr};
  sensor::Sensor *delay_bathroom_switch_on{nullptr};
  sensor::Sensor *delay_bathroom_switch_off{nullptr};
  sensor::Sensor *delay_boost{nullptr};
  sensor::Sensor *delay_filter_weeks{nullptr};
  sensor::Sensor *analog_input_1{nullptr};
  sensor::Sensor *analog_input_2{nullptr};
  sensor::Sensor *analog_input_3{nullptr};
  sensor::Sensor *analog_input_4{nullptr};
  sensor::Sensor *ewt_state{nullptr};
  sensor::Sensor *reheater_state{nullptr};
  sensor::Sensor *ewt_temperature_low{nullptr};
  sensor::Sensor *ewt_temperature_high{nullptr};

  void set_fan_supply_percentage(sensor::Sensor *fan_supply_percentage) {this->fan_supply_percentage = fan_supply_percentage;};
  void set_fan_exhaust_percentage(sensor::Sensor *fan_exhaust_percentage) {this->fan_exhaust_percentage =fan_exhaust_percentage; };
  void set_fan_speed_supply(sensor::Sensor *fan_speed_supply) {this->fan_speed_supply =fan_speed_supply; };
  void set_fan_speed_exhaust(sensor::Sensor *fan_speed_exhaust) {this->fan_speed_exhaust =fan_speed_exhaust; };
  void set_is_bypass_valve_open(binary_sensor::BinarySensor *is_bypass_valve_open) {this->is_bypass_valve_open =is_bypass_valve_open; };
  void set_is_preheating(binary_sensor::BinarySensor *is_preheating) {this->is_preheating =is_preheating; };
  void set_outside_air_temperature(sensor::Sensor *outside_air_temperature) {this->outside_air_temperature =outside_air_temperature; };
  void set_supply_air_temperature(sensor::Sensor *supply_air_temperature) {this->supply_air_temperature =supply_air_temperature; };
  void set_return_air_temperature(sensor::Sensor *return_air_temperature) {this->return_air_temperature =return_air_temperature; };
  void set_exhaust_air_temperature(sensor::Sensor *exhaust_air_temperature) {this->exhaust_air_temperature =exhaust_air_temperature; };
  void set_enthalpy_temperature(sensor::Sensor *enthalpy_temperature) {this->enthalpy_temperature =enthalpy_temperature; };
  void set_ewt_temperature(sensor::Sensor *ewt_temperature) {this->ewt_temperature =ewt_temperature; };
  void set_reheating_temperature(sensor::Sensor *reheating_temperature) {this->reheating_temperature =reheating_temperature; };
  void set_kitchen_hood_temperature(sensor::Sensor *kitchen_hood_temperature) {this->kitchen_hood_temperature =kitchen_hood_temperature; };
  void set_return_air_level(sensor::Sensor *return_air_level) {this->return_air_level =return_air_level; };
  void set_supply_air_level(sensor::Sensor *supply_air_level) {this->supply_air_level =supply_air_level; };
  void set_is_supply_fan_active(binary_sensor::BinarySensor *is_supply_fan_active) {this->is_supply_fan_active =is_supply_fan_active; };
  void set_is_filter_full(binary_sensor::BinarySensor *is_filter_full) {this->is_filter_full =is_filter_full; };
  void set_bypass_factor(sensor::Sensor *bypass_factor) {this->bypass_factor = bypass_factor; };
  void set_bypass_step(sensor::Sensor *bypass_step) {this->bypass_step = bypass_step; };
  void set_bypass_correction(sensor::Sensor *bypass_correction) {this->bypass_correction = bypass_correction; };
  void set_is_summer_mode(binary_sensor::BinarySensor *is_summer_mode) {this->is_summer_mode = is_summer_mode; };
  void set_fan_exhaust_absent(sensor::Sensor *s) { this->fan_exhaust_absent = s; };
  void set_fan_exhaust_low(sensor::Sensor *s) { this->fan_exhaust_low = s; };
  void set_fan_exhaust_medium(sensor::Sensor *s) { this->fan_exhaust_medium = s; };
  void set_fan_exhaust_high(sensor::Sensor *s) { this->fan_exhaust_high = s; };
  void set_fan_supply_absent(sensor::Sensor *s) { this->fan_supply_absent = s; };
  void set_fan_supply_low(sensor::Sensor *s) { this->fan_supply_low = s; };
  void set_fan_supply_medium(sensor::Sensor *s) { this->fan_supply_medium = s; };
  void set_fan_supply_high(sensor::Sensor *s) { this->fan_supply_high = s; };
  void set_fan_hours_supply(sensor::Sensor *fan_hours_supply) {this->fan_hours_supply = fan_hours_supply; };
  void set_fan_hours_exhaust(sensor::Sensor *fan_hours_exhaust) {this->fan_hours_exhaust = fan_hours_exhaust; };
  void set_bypass_hours(sensor::Sensor *bypass_hours) {this->bypass_hours = bypass_hours; };
  void set_preheater_hours(sensor::Sensor *preheater_hours) {this->preheater_hours = preheater_hours; };
  void set_filter_hours(sensor::Sensor *filter_hours) {this->filter_hours = filter_hours; };
  void set_ewt_hours(sensor::Sensor *ewt_hours) {this->ewt_hours = ewt_hours; };
  void set_error_code(sensor::Sensor *s) { this->error_code = s; };
  void set_is_supply_fan_fault(binary_sensor::BinarySensor *s) { this->is_supply_fan_fault = s; };
  void set_is_exhaust_fan_fault(binary_sensor::BinarySensor *s) { this->is_exhaust_fan_fault = s; };
  void set_is_t1_sensor_fault(binary_sensor::BinarySensor *s) { this->is_t1_sensor_fault = s; };
  void set_is_t2_sensor_fault(binary_sensor::BinarySensor *s) { this->is_t2_sensor_fault = s; };
  void set_is_t3_sensor_fault(binary_sensor::BinarySensor *s) { this->is_t3_sensor_fault = s; };
  void set_is_t4_sensor_fault(binary_sensor::BinarySensor *s) { this->is_t4_sensor_fault = s; };
  void set_is_bypass_fault(binary_sensor::BinarySensor *s) { this->is_bypass_fault = s; };
  void set_is_preheater_fault(binary_sensor::BinarySensor *s) { this->is_preheater_fault = s; };
  void set_delay_bathroom_switch_on(sensor::Sensor *s) { this->delay_bathroom_switch_on = s; };
  void set_delay_bathroom_switch_off(sensor::Sensor *s) { this->delay_bathroom_switch_off = s; };
  void set_delay_boost(sensor::Sensor *s) { this->delay_boost = s; };
  void set_delay_filter_weeks(sensor::Sensor *s) { this->delay_filter_weeks = s; };
  void set_analog_input_1(sensor::Sensor *s) { this->analog_input_1 = s; };
  void set_analog_input_2(sensor::Sensor *s) { this->analog_input_2 = s; };
  void set_analog_input_3(sensor::Sensor *s) { this->analog_input_3 = s; };
  void set_analog_input_4(sensor::Sensor *s) { this->analog_input_4 = s; };
  void set_ewt_state(sensor::Sensor *s) { this->ewt_state = s; };
  void set_reheater_state(sensor::Sensor *s) { this->reheater_state = s; };
  void set_ewt_temperature_low(sensor::Sensor *s) { this->ewt_temperature_low = s; };
  void set_ewt_temperature_high(sensor::Sensor *s) { this->ewt_temperature_high = s; };
  switch_::Switch *auto_balance_switch_{nullptr};
  void set_auto_balance_switch(switch_::Switch *sw) { this->auto_balance_switch_ = sw; }
};

inline void ComfoAirAutoBalanceSwitch::write_state(bool state) {
  ESP_LOGD("comfoair.switch", "Auto Balance switch turned %s", state ? "ON" : "OFF");
  this->publish_state(state);
  if (this->parent_ != nullptr) {
      this->parent_->run_auto_balance_(true);
  }
}

}  // namespace comfoair
}  // namespace esphome
