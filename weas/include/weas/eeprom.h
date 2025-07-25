
#ifndef WEAS_EEPROM_H
#define WEAS_EEPROM_H

#include "hardware/gpio.h"

class EEPROM {

public:

	enum USBPowerState {DFP, UFP, Unsupported};
	enum HardwareVersion {Proto1=0x2a, Proto2_Rev1=0x30, Rev1_1=0x0C, Unknown=0xFF};

	static constexpr uint USB_HOST_STATUS = 20;
	static constexpr uint BOARD_ID_0 = 7;
	static constexpr uint BOARD_ID_1 = 6;
	static constexpr uint BOARD_ID_2 = 5;
	static constexpr uint EEPROM_ADDR_ID = 0;
	static constexpr uint EEPROM_ADDR_VERSION = 2;
	static constexpr uint EEPROM_ADDR_CRC_L = 87;
	static constexpr uint EEPROM_ADDR_CRC_H = 86;
	static constexpr uint EEPROM_VAL_ID = 2001;
	static constexpr uint EEPROM_NUM_BYTES = 88;
	static constexpr uint EEPROM_PAGE_ADDRESS = 0x50;

  EEPROM(const EEPROM&) = delete;
  EEPROM& operator=(const EEPROM&) = delete;

	USBPowerState get_usb_power_state()	{
		if (get_hardware_version() != Rev1_1) return Unsupported;
		if (gpio_get(USB_HOST_STATUS)) return UFP;
		return DFP;
	}
	HardwareVersion get_hardware_version() {return hw;}
	uint64_t get_unique_id()	{return unique_id;}

private:

  EEPROM() {

  	// Board version ID pins
  	gpio_init(BOARD_ID_0);
  	gpio_init(BOARD_ID_1);
  	gpio_init(BOARD_ID_2);
  	gpio_set_dir(BOARD_ID_0, GPIO_IN);
  	gpio_set_dir(BOARD_ID_1, GPIO_IN);
  	gpio_set_dir(BOARD_ID_2, GPIO_IN);
  	hw = probe_hardware_version();

  	// USB host status pin
  	gpio_init(USB_HOST_STATUS);
  	gpio_disable_pulls(USB_HOST_STATUS);

  	// Read EEPROM calibration values
  	read_eeprom();

  	// Read unique card ID
  	flash_get_unique_id((uint8_t *) &unique_id);
  	// Do some mixing up of the bits using full-cycle 64-bit LCG
  	// Should help ensure most bytes change even if many bits of
  	// the original flash unique ID are the same between flash chips.
  	for (int i=0; i<20; i++)
  	{
  		unique_id = unique_id * 6364136223846793005ULL + 1442695040888963407ULL;
  	}

  }

	typedef struct {
		float m, b;
		int32_t mi, bi;
	} CalCoeffs;

	typedef struct {
		int32_t dacSetting;
		int8_t voltage;
	} CalPoint;

	static constexpr uint CAL_MAX_CHANNELS = 2;  // TODO - Channel?
	static constexpr int CAL_MAX_POINTS = 10;

	uint8_t num_calibration_points[CAL_MAX_CHANNELS];
	CalPoint calibration_table[CAL_MAX_CHANNELS][CAL_MAX_POINTS];
	CalCoeffs cal_coeffs[CAL_MAX_CHANNELS];

	uint64_t unique_id;

	uint8_t read_byte_from_eeprom(uint eeAddress);
	int read_int_from_eeprom(uint eeAddress);
	uint16_t crc_encode(const uint8_t *data, uint length);
	void calc_cal_coeffs(uint channel);
	int read_eeprom();

	HardwareVersion hw;
	HardwareVersion probe_hardware_version();

	uint32_t midi_to_dac(int midiNote, int channel);

};


EEPROM::HardwareVersion EEPROM::probe_hardware_version()
{
  // Enable pull-downs, and measure
  gpio_set_pulls(BOARD_ID_0, false, true);
  gpio_set_pulls(BOARD_ID_1, false, true);
  gpio_set_pulls(BOARD_ID_2, false, true);
  sleep_us(1);

  // Pull-down state in bits 0, 2, 4
  uint8_t pd = gpio_get(BOARD_ID_0) | (gpio_get(BOARD_ID_1) << 2) | (gpio_get(BOARD_ID_2) << 4);

  // Enable pull-ups, and measure
  gpio_set_pulls(BOARD_ID_0, true, false);
  gpio_set_pulls(BOARD_ID_1, true, false);
  gpio_set_pulls(BOARD_ID_2, true, false);
  sleep_us(1);

  // Pull-up state in bits 1, 3, 5
  uint8_t pu = (gpio_get(BOARD_ID_0) << 1) | (gpio_get(BOARD_ID_1) << 3) | (gpio_get(BOARD_ID_2) << 5);

  // Combine to give 6-bit ID
  uint8_t id = pd | pu;

  // Set pull-downs
  gpio_set_pulls(BOARD_ID_0, false, true);
  gpio_set_pulls(BOARD_ID_1, false, true);
  gpio_set_pulls(BOARD_ID_2, false, true);

  switch (id)
  {
  case Proto1:
  case Proto2_Rev1:
  case Rev1_1:
    return static_cast<HardwareVersion>(id);
  default:
    return Unknown;
  }
}


uint8_t EEPROM::read_byte_from_eeprom(unsigned int eeAddress)
{
	uint8_t deviceAddress = EEPROM_PAGE_ADDRESS | ((eeAddress >> 8) & 0x0F);
	uint8_t data = 0xFF;

	uint8_t addr_low_byte = eeAddress & 0xFF;
	i2c_write_blocking(i2c0, deviceAddress, &addr_low_byte, 1, false);

	i2c_read_blocking(i2c0, deviceAddress, &data, 1, false);
	return data;
}

int EEPROM::read_int_from_eeprom(unsigned int eeAddress)
{
	uint8_t highByte = read_byte_from_eeprom(eeAddress);
	uint8_t lowByte = read_byte_from_eeprom(eeAddress + 1);
	return (highByte << 8) | lowByte;
}

uint16_t EEPROM::crc_encode(const uint8_t *data, uint length)
{
	uint16_t crc = 0xFFFF; // Initial CRC value
	for (uint i = 0; i < length; i++)
	{
		crc ^= ((uint16_t)data[i]) << 8; // Bring in the next byte
		for (uint8_t bit = 0; bit < 8; bit++)
		{
			if (crc & 0x8000)
			{
				crc = (crc << 1) ^ 0x1021; // CRC-CCITT polynomial
			}
			else
			{
				crc = crc << 1;
			}
		}
	}
	return crc;
}


int EEPROM::read_eeprom()
{
	// Set up default values in the calibration table,
	// to be used if EEPROM read fails
	calibration_table[0][0].voltage = -20; // -2V
	calibration_table[0][0].dacSetting = 347700;
	calibration_table[0][1].voltage = 0; // 0V
	calibration_table[0][1].dacSetting = 261200;
	calibration_table[0][2].voltage = 20; // +2V
	calibration_table[0][2].dacSetting = 174400;

	calibration_table[1][0].voltage = -20; // -2V
	calibration_table[1][0].dacSetting = 347700;
	calibration_table[1][1].voltage = 0; // 0V
	calibration_table[1][1].dacSetting = 261200;
	calibration_table[1][2].voltage = 20; // +2V
	calibration_table[1][2].dacSetting = 174400;

	if (read_int_from_eeprom(EEPROM_ADDR_ID) != EEPROM_VAL_ID)
	{
		return 1;
	}
	uint8_t buf[EEPROM_NUM_BYTES];
	for (uint i = 0; i < EEPROM_NUM_BYTES; i++)
	{
		buf[i] = read_byte_from_eeprom(i);
	}


	uint16_t calculatedCRC = crc_encode(buf, 86u);
	uint16_t foundCRC = ((uint16_t)buf[EEPROM_ADDR_CRC_H] << 8) | buf[EEPROM_ADDR_CRC_L];

	if (calculatedCRC != foundCRC)
	{
		return 1;
	}

	int bufferIndex = 4;

	for (uint8_t channel = 0; channel < CAL_MAX_CHANNELS; channel++)
	{
		int channelOffset = bufferIndex + (41 * channel); // channel 0 = 4, channel 1 = 45
		num_calibration_points[channel] = buf[channelOffset++];
		for (uint8_t point = 0; point < num_calibration_points[channel]; point++)
		{
			// Unpack Pack targetVoltage (int8_t) from buf
			int8_t targetVoltage = (int8_t)buf[channelOffset++];

			// Unack dacSetting (uint32_t) from buf (4 bytes)
			uint32_t dacSetting = 0;
			dacSetting |= ((uint32_t)buf[channelOffset++]) << 24; // MSB
			dacSetting |= ((uint32_t)buf[channelOffset++]) << 16;
			dacSetting |= ((uint32_t)buf[channelOffset++]) << 8;
			dacSetting |= ((uint32_t)buf[channelOffset++]); // LSB

			// Write settings into calibration table
			calibration_table[channel][point].voltage = targetVoltage;
			calibration_table[channel][point].dacSetting = dacSetting;
		}
		calc_cal_coeffs(channel);
	}

	return 0;
}

void EEPROM::calc_cal_coeffs(uint channel)
{
	float sumV = 0.0;
	float sumDAC = 0.0;
	float sumV2 = 0.0;
	float sumVDAC = 0.0;
	int N = num_calibration_points[channel];

	for (int i = 0; i < N; i++)
	{
		float v = calibration_table[channel][i].voltage * 0.1f;
		float dac = calibration_table[channel][i].dacSetting;
		sumV += v;
		sumDAC += dac;
		sumV2 += v * v;
		sumVDAC += v * dac;
	}

	float denominator = N * sumV2 - sumV * sumV;
	if (denominator != 0)
	{
		cal_coeffs[channel].m = (N * sumVDAC - sumV * sumDAC) / denominator;
	}
	else
	{
		cal_coeffs[channel].m = 0.0;
	}
	cal_coeffs[channel].b = (sumDAC - cal_coeffs[channel].m * sumV) / N;

	cal_coeffs[channel].mi = int32_t(cal_coeffs[channel].m * 1.333333333333333f + 0.5f);
	cal_coeffs[channel].bi = int32_t(cal_coeffs[channel].b + 0.5f);
}

uint32_t EEPROM::midi_to_dac(int midiNote, int channel) {
	int32_t dacValue = ((cal_coeffs[channel].mi * (midiNote - 60)) >> 4) + cal_coeffs[channel].bi;
	if (dacValue > 524287) dacValue = 524287;
	if (dacValue < 0) dacValue = 0;
	return dacValue;
}

#endif
