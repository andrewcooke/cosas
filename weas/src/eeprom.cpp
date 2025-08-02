
#include "weas/eeprom.h"

#include "hardware/flash.h"


EEPROM::EEPROM() {

  gpio_init(BOARD_ID_0);
  gpio_init(BOARD_ID_1);
  gpio_init(BOARD_ID_2);
  gpio_set_dir(BOARD_ID_0, GPIO_IN);
  gpio_set_dir(BOARD_ID_1, GPIO_IN);
  gpio_set_dir(BOARD_ID_2, GPIO_IN);
  hw = probe_hardware_version();

  gpio_init(USB_HOST_STATUS);
  gpio_disable_pulls(USB_HOST_STATUS);

  read_eeprom();
  flash_get_unique_id((uint8_t*)&unique_id);

  // Do some mixing up of the bits using full-cycle 64-bit LCG
  // Should help ensure most bytes change even if many bits of
  // the original flash unique ID are the same between flash chips.
  for (uint i = 0; i < 20; i++) {
    unique_id = unique_id * 6364136223846793005ull + 1442695040888963407ull;
  }
}

EEPROM& EEPROM::get() {
  static EEPROM eeprom;
  return eeprom;
}

EEPROM::USBPowerState EEPROM::get_usb_power_state()	{
  if (get_hardware_version() != Rev1_1) return Unsupported;
  if (gpio_get(USB_HOST_STATUS)) return UFP;
  return DFP;
}

EEPROM::HardwareVersion EEPROM::probe_hardware_version() {
  gpio_set_pulls(BOARD_ID_0, false, true);
  gpio_set_pulls(BOARD_ID_1, false, true);
  gpio_set_pulls(BOARD_ID_2, false, true);
  sleep_us(1);
  uint8_t pd = gpio_get(BOARD_ID_0) | (gpio_get(BOARD_ID_1) << 2) | (gpio_get(BOARD_ID_2) << 4);

  gpio_set_pulls(BOARD_ID_0, true, false);
  gpio_set_pulls(BOARD_ID_1, true, false);
  gpio_set_pulls(BOARD_ID_2, true, false);
  sleep_us(1);
  uint8_t pu = (gpio_get(BOARD_ID_0) << 1) | (gpio_get(BOARD_ID_1) << 3) | (gpio_get(BOARD_ID_2) << 5);

  uint8_t id = pd | pu;

  gpio_set_pulls(BOARD_ID_0, false, true);
  gpio_set_pulls(BOARD_ID_1, false, true);
  gpio_set_pulls(BOARD_ID_2, false, true);

  switch (id) {
  case Proto1:
  case Proto2_Rev1:
  case Rev1_1:
    return static_cast<HardwareVersion>(id);
  default:
    return Unknown;
  }
}


uint8_t EEPROM::read_byte_from_eeprom(unsigned int ee_addr) {
  uint8_t device_addr = EEPROM_PAGE_ADDRESS | ((ee_addr >> 8) & 0x0F);
  uint8_t data = 0xFF;
  uint8_t addr_low_byte = ee_addr & 0xFF;
  i2c_write_blocking(i2c0, device_addr, &addr_low_byte, 1, false);
  i2c_read_blocking(i2c0, device_addr, &data, 1, false);
  return data;
}

int EEPROM::read_int_from_eeprom(unsigned int ee_addr) {
  uint8_t highByte = read_byte_from_eeprom(ee_addr);
  uint8_t lowByte = read_byte_from_eeprom(ee_addr + 1);
  return (highByte << 8) | lowByte;
}

uint16_t EEPROM::crc_encode(const uint8_t* data, uint length) {
  uint16_t crc = 0xFFFF;
  for (uint i = 0; i < length; i++) {
    crc ^= ((uint16_t)data[i]) << 8;
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021; // CRC-CCITT polynomial
      } else {
        crc = crc << 1;
      }
    }
  }
  return crc;
}

int EEPROM::read_eeprom() {
  for (uint chan = 0; chan < N_CHANNELS; chan++) {
    calibration_table[chan][0].voltage = -20; // -2V
    calibration_table[chan][0].dacSetting = 347700;
    calibration_table[chan][1].voltage = 0; // 0V
    calibration_table[chan][1].dacSetting = 261200;
    calibration_table[chan][2].voltage = 20; // +2V
    calibration_table[chan][2].dacSetting = 174400;
  }

  if (read_int_from_eeprom(EEPROM_ADDR_ID) != EEPROM_VAL_ID) return 1;
  uint8_t buf[EEPROM_NUM_BYTES];
  for (uint i = 0; i < EEPROM_NUM_BYTES; i++) {
    buf[i] = read_byte_from_eeprom(i);
  }

  uint16_t calculated_crc = crc_encode(buf, 86u);
  uint16_t found_crc = ((uint16_t)buf[EEPROM_ADDR_CRC_H] << 8) | buf[EEPROM_ADDR_CRC_L];

  if (calculated_crc != found_crc) return 1;

  int buffer_index = 4;

  for (uint8_t chan = 0; chan < N_CHANNELS; chan++) {
    int chan_offset = buffer_index + (41 * chan); // TODO - 41 seems like it's a block size?  constant?
    num_calibration_points[chan] = buf[chan_offset++];
    for (uint8_t point = 0; point < num_calibration_points[chan]; point++) {
      int8_t target_voltage = (int8_t)buf[chan_offset++];
      uint32_t dac_setting = 0;
      dac_setting |= ((uint32_t)buf[chan_offset++]) << 24; // MSB
      dac_setting |= ((uint32_t)buf[chan_offset++]) << 16;
      dac_setting |= ((uint32_t)buf[chan_offset++]) << 8;
      dac_setting |= ((uint32_t)buf[chan_offset++]); // LSB

      calibration_table[chan][point].voltage = target_voltage;
      calibration_table[chan][point].dacSetting = dac_setting;
    }
    calc_cal_coeffs(chan);
  }

  return 0;
}

void EEPROM::calc_cal_coeffs(uint channel) {
  float sum_v = 0.0;
  float sum_dac = 0.0;
  float sum_v2 = 0.0;
  float sum_vdac = 0.0;
  uint n = num_calibration_points[channel];

  for (uint i = 0; i < n; i++) {
    float v = calibration_table[channel][i].voltage * 0.1f;
    float dac = calibration_table[channel][i].dacSetting;
    sum_v += v;
    sum_dac += dac;
    sum_v2 += v * v;
    sum_vdac += v * dac;
  }

  float denominator = n * sum_v2 - sum_v * sum_v;
  if (denominator != 0) {
    cal_coeffs[channel].m = (n * sum_vdac - sum_v * sum_dac) / denominator;
  } else {
    cal_coeffs[channel].m = 0.0;
  }
  cal_coeffs[channel].b = (sum_dac - cal_coeffs[channel].m * sum_v) / n;
  cal_coeffs[channel].mi = static_cast<int32_t>(cal_coeffs[channel].m * 1.333333333333333f + 0.5f);
  cal_coeffs[channel].bi = static_cast<int32_t>(cal_coeffs[channel].b + 0.5f);
}

uint32_t __not_in_flash_func(EEPROM::midi_to_dac)(Channel lr, uint note) {
  int32_t dacValue = ((cal_coeffs[lr].mi * (note - 60)) >> 4) + cal_coeffs[lr].bi;
  if (dacValue > 524287) dacValue = 524287; // 19 bits
  if (dacValue < 0) dacValue = 0;
  return dacValue;
}

uint32_t __not_in_flash_func(EEPROM::midi_to_dac)(uint lr, uint note) {
  return midi_to_dac(static_cast<Channel>(lr), note);
}

void __not_in_flash_func(EEPROM::write_cv_midi_note)(Codec& cc, Channel lr, uint8_t note) {
  cc.write_cv(lr, midi_to_dac(lr, note));
}

void __not_in_flash_func(EEPROM::write_cv_midi_note)(Codec& cc, uint lr, uint8_t note) {
  write_cv_midi_note(cc, static_cast<Channel>(lr), note);
}
