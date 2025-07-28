
#ifndef WEAS_EEPROM_H
#define WEAS_EEPROM_H

#include "hardware/gpio.h"

#include "weas/weas.h"
#include "weas/codec.h"


// the interface to the eeprom and related functions
// midi output is here because it needs calibration data
// (presumably to generate the correct notes!)


class EEPROM {

public:

	enum USBPowerState {DFP, UFP, Unsupported};
	enum HardwareVersion {Proto1=0x2a, Proto2_Rev1=0x30, Rev1_1=0x0C, Unknown=0xFF};

	static EEPROM& get();
  EEPROM(const EEPROM&) = delete;
  EEPROM& operator=(const EEPROM&) = delete;

	USBPowerState get_usb_power_state();
	HardwareVersion get_hardware_version() const {return hw;}
	uint64_t get_unique_id() const {return unique_id;}

	uint32_t midi_to_dac(int midiNote, int channel);
	template<uint O, uint S> void write_cv_midi_note(CC<O, S> cc, Channel lr, uint8_t note_num);
	template<uint O, uint S> void write_cv_midi_note(CC<O, S> cc, uint lr, uint8_t note_num);

private:

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

	EEPROM();
	
	typedef struct {
		float m, b;
		int32_t mi, bi;
	} CalCoeffs;

	typedef struct {
		int32_t dacSetting;
		int8_t voltage;
	} CalPoint;

	static constexpr int CAL_MAX_POINTS = 10;

	uint8_t num_calibration_points[N_CHANNELS];
	CalPoint calibration_table[N_CHANNELS][CAL_MAX_POINTS];
	CalCoeffs cal_coeffs[N_CHANNELS];
	uint64_t unique_id;
	HardwareVersion hw;

	uint8_t read_byte_from_eeprom(uint ee_addr);
	int read_int_from_eeprom(uint ee_addr);
	uint16_t crc_encode(const uint8_t *data, uint length);
	void calc_cal_coeffs(uint channel);
	int read_eeprom();
	HardwareVersion probe_hardware_version();
};


#endif
