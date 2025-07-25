
#ifndef WEAS_CC_H
#define WEAS_CC_H

#include <functional>

#include "hardware/gpio.h"
#include "hardware/pwm.h"

#include "cosas/dnl.h"


// common values for SAMPLE_FREQ
static constexpr uint CC_SAMPLE_44_1 = 44100;
static constexpr uint CC_SAMPLE_8 = 48000;


template<uint OVERSAMPLE_BITS, uint SAMPLE_FREQ> class CC final {

public:

	enum Channel {Left, Right};
	enum Knob {Main, X, Y};
	enum Switch {Down, Middle, Up};
	enum Input {Audio1, Audio2, CV1, CV2, Pulse1, Pulse2};
	enum HardwareVersion {Proto1=0x2a, Proto2_Rev1=0x30, Rev1_1=0x0C, Unknown=0xFF};
	enum USBPowerState {DFP, UFP, Unsupported};

	static constexpr uint N_CHANNELS = Channel::Right + 1;
	static constexpr uint N_KNOBS = Knob::Y + 2;  // switch is stored here too
	static constexpr uint ADC_MAX_UNCORRECTED = 4127;

	static CC& get() {
		static CC cc;
		return cc;
	}

	void run() {audio_worker();}  // TODO - just rename audio_worker to run; TODO - add blocking
	void abort();  // TODO - example of use?

	[[nodiscard]] int32_t get_count() const {return count;}
	void set_normalisation_probe(bool use) {use_norm_probe = use;}
	void set_per_sample(std::function<void(CC&)> f) {per_sample = f;}

	[[nodiscard]] uint16_t __not_in_flash_func(read_knob)(Knob k) {return knobs[k];}
	[[nodiscard]] uint16_t __not_in_flash_func(read_knob)(uint k) {return read_knob(static_cast<Knob>(k));}
	[[nodiscard]] Switch __not_in_flash_func(read_switch)() {return switch_;}
	[[nodiscard]] bool __not_in_flash_func(switch_changed)() {return switch_ != prev_switch;}

	// TODO - silly separate vars
	[[nodiscard]] int16_t __not_in_flash_func(read_audio)(Channel lr) {return lr ? adcInR:adcInL;}
	[[nodiscard]] int16_t __not_in_flash_func(read_audio)(uint lr) {return read_audio(static_cast<Channel>(lr));}
	void __not_in_flash_func(write_audio)(Channel lr, int16_t v) {dac[lr] = v;}
	void __not_in_flash_func(write_audio)(uint lr, int16_t v) {write_audio(static_cast<Channel>(lr), v);}

	[[nodiscard]] int16_t __not_in_flash_func(read_cv)(Channel lr) {return cv[lr];}
	[[nodiscard]] int16_t __not_in_flash_func(read_cv)(uint lr) {return read_cv(static_cast<Channel>(lr));}
	// discard a bit because pwm 11 bits for reduced ripple
	// cv pins in reverse order
	void __not_in_flash_func(write_cv)(Channel lr, int16_t v) {pwm_set_gpio_level(CV_OUT_1 - lr, (0x7ff - v) >> 1);}
	void __not_in_flash_func(write_cv)(uint lr, int16_t v) {write_cv(static_cast<Channel>(lr), v);}

	// TODO - candidate for separate class?
	void __not_in_flash_func(write_cv_midi_note)(Channel lr, uint8_t note_num) {
		pwm_set_gpio_level(CV_OUT_1 - lr, midi_to_dac(note_num, lr) >> 8);
	}
	void __not_in_flash_func(write_cv_midi_note)(uint lr, uint8_t note_num) {
		write_cv_midi_note(static_cast<Channel>(lr), note_num);
	}

	[[nodiscard]] bool __not_in_flash_func(read_pulse)(Channel lr) {return pulse[lr];}
	[[nodiscard]] bool __not_in_flash_func(read_pulse)(uint lr) {return read_pulse(static_cast<Channel>(lr));}
	void __not_in_flash_func(write_pulse)(Channel lr, bool v) {gpio_put(PULSE_1_RAW_OUT + lr, !v);}
	void __not_in_flash_func(write_pulse)(uint lr, bool v) {write_pulse(static_cast<Channel>(lr), v);}
	// TODO - merge separate arrays
	[[nodiscard]] bool __not_in_flash_func(pulse_rose)(Channel lr) {return pulse[lr] && !last_pulse[lr];}
	[[nodiscard]] bool __not_in_flash_func(pulse_rose)(uint lr) {return pulse_rose(static_cast<Channel>(lr));}
	[[nodiscard]] bool __not_in_flash_func(pulse_fell)(Channel lr) {return !pulse[lr] && last_pulse[lr];}
	[[nodiscard]] bool __not_in_flash_func(pulse_fell)(uint lr) {return pulse_fell(static_cast<Channel>(lr));}

	[[nodiscard]] bool __not_in_flash_func(is_connected)(Input i) {return connected[i];}

	// TODO - candidate for separate class?
	USBPowerState get_usb_power_state()	{
		if (get_hardware_version() != Rev1_1) return Unsupported;
		if (gpio_get(USB_HOST_STATUS)) return UFP;
	  return DFP;
	}
	HardwareVersion get_hardware_version() {return hw;}
	uint64_t get_unique_id()	{return unique_id;}


private:

	static constexpr uint OVERSAMPLES = 1 << OVERSAMPLE_BITS;

	static constexpr uint PULSE_1_RAW_OUT = 8;
	static constexpr uint PULSE_2_RAW_OUT = 9;
	static constexpr uint CV_OUT_1 = 23;
	static constexpr uint CV_OUT_2 = 22;
	static constexpr uint USB_HOST_STATUS = 20;

	// TODO - check what a singleton expects here
	CC();
	~CC() = default;
	std::function<void(CC&)> per_sample = [](CC&){};
	int32_t count = 0;


	// TODO - candidates for separate class ------------------------------------------------

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
	HardwareVersion ProbeHardwareVersion();

	uint32_t midi_to_dac(int midiNote, int channel);
	
	int16_t dac[N_CHANNELS];  // cv output
	volatile int32_t knobs[N_KNOBS] = {};
	volatile bool pulse[2] = {};
	volatile bool last_pulse[2] = { 0, 0 };
	volatile int32_t cv[2] = { 0, 0 };
	volatile int16_t adcInL = 0x800, adcInR = 0x800;

	volatile uint8_t mxPos = 0;

	volatile int32_t plug_state[6] = {0,0,0,0,0,0};
	volatile bool connected[6] = {0,0,0,0,0,0};
	bool use_norm_probe;

	Switch switch_, prev_switch;
	
	volatile uint8_t runADCMode;

	uint16_t ADC_Buffer[2][4 * OVERSAMPLES];
	uint16_t SPI_Buffer[2][2];

	uint8_t adc_dma, spi_dma;

	uint8_t dmaPhase = 0;

	uint16_t __not_in_flash_func(dacval)(int16_t value, uint16_t dacChannel) {
		if (value < -2048) value = -2048;
		if (value > 2047) value = 2047;
		return (dacChannel | 0x3000) | (((uint16_t)((value & 0x0FFF) + 0x800)) & 0x0FFF);
	}
	
	uint32_t next_norm_probe();
	void BufferFull();
	void audio_worker();
	
	static void AudioCallback()	{
		CC& cc = CC::get();
		cc.BufferFull();
	}

};

#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/flash.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/spi.h"

// Input normalisation probe pin
#define NORMALISATION_PROBE 4

// Mux pins
#define MX_A 24
#define MX_B 25

// ADC input pins
#define AUDIO_L_IN_1 27
#define AUDIO_R_IN_1 26
#define MUX_IO_1 28
#define MUX_IO_2 29

#define DAC_CHANNEL_A 0x0000
#define DAC_CHANNEL_B 0x8000

#define DAC_CS 21
#define DAC_SCK 18
#define DAC_TX 19

#define EEPROM_SDA 16
#define EEPROM_SCL 17

#define PULSE_1_INPUT 2
#define PULSE_2_INPUT 3

#define DEBUG_1 0
#define DEBUG_2 1

#define SPI_PORT spi0
#define SPI_DREQ DREQ_SPI0_TX


#define BOARD_ID_0 7
#define BOARD_ID_1 6
#define BOARD_ID_2 5

// The ADC (/DMA) run mode, used to stop DMA in a known state before writing to flash
#define RUN_ADC_MODE_RUNNING 0
#define RUN_ADC_MODE_REQUEST_ADC_STOP 1
#define RUN_ADC_MODE_ADC_STOPPED 2
#define RUN_ADC_MODE_REQUEST_ADC_RESTART 3


#define EEPROM_ADDR_ID 0
#define EEPROM_ADDR_VERSION 2
#define EEPROM_ADDR_CRC_L 87
#define EEPROM_ADDR_CRC_H 86
#define EEPROM_VAL_ID 2001
#define EEPROM_NUM_BYTES 88

#define EEPROM_PAGE_ADDRESS 0x50


// Return pseudo-random bit for normalisation probe
template<uint O, uint F> uint32_t __attribute__((section(".time_critical." "cc-next-norm-probe")))
CC<O, F>::next_norm_probe()
{
	static uint32_t lcg_seed = 1;
	lcg_seed = 1664525 * lcg_seed + 1013904223;
	return lcg_seed >> 31;
}

// Main audio core function
template<uint O, uint SAMPLE_FREQ> __attribute__((section(".time_critical." "cc-audio-worker")))
void (CC<O, SAMPLE_FREQ>::audio_worker)()
{

	adc_select_input(0);
	adc_set_round_robin(0b0001111U);

	// enabled, with DMA request when FIFO contains data, no erro flag, no byte shift
	adc_fifo_setup(true, true, 1, false, false);


	// ADC clock runs at 48MHz
	// 48MHz ÷ (124+1) = 384kHz ADC sample rate
	//                 = 8×48kHz audio sample rate
	adc_set_clkdiv((48000000 / (SAMPLE_FREQ * 4 * OVERSAMPLES)) - 1);

	// claim and setup DMAs for reading to ADC, and writing to SPI DAC
	adc_dma = dma_claim_unused_channel(true);
	spi_dma = dma_claim_unused_channel(true);

	dma_channel_config adc_dmacfg, spi_dmacfg;
	adc_dmacfg = dma_channel_get_default_config(adc_dma);
	spi_dmacfg = dma_channel_get_default_config(spi_dma);

	// Reading from ADC into memory buffer, so increment on write, but no increment on read
	channel_config_set_transfer_data_size(&adc_dmacfg, DMA_SIZE_16);
	channel_config_set_read_increment(&adc_dmacfg, false);
	channel_config_set_write_increment(&adc_dmacfg, true);

	// Synchronise ADC DMA the ADC samples
	channel_config_set_dreq(&adc_dmacfg, DREQ_ADC);

	// Setup DMA for 4 * OVERSAMPLES ADC samples
	dma_channel_configure(adc_dma, &adc_dmacfg, ADC_Buffer[dmaPhase], &adc_hw->fifo, 4 * OVERSAMPLES, true);

	// Turn on IRQ for ADC DMA
	dma_channel_set_irq0_enabled(adc_dma, true);

	// Call buffer_full ISR when ADC DMA finished
	irq_set_enabled(DMA_IRQ_0, true);
	irq_set_exclusive_handler(DMA_IRQ_0, CC::AudioCallback);



	// Set up DMA for SPI
	spi_dmacfg = dma_channel_get_default_config(spi_dma);
	channel_config_set_transfer_data_size(&spi_dmacfg, DMA_SIZE_16);

	// SPI DMA timed to SPI TX
	channel_config_set_dreq(&spi_dmacfg, SPI_DREQ);

	// Set up DMA to transmit 2 samples to SPI
	dma_channel_configure(spi_dma, &spi_dmacfg, &spi_get_hw(SPI_PORT)->dr, NULL, 2, false);

	adc_run(true);

	while (1)
	{
		// If ready to restart
		if (runADCMode == RUN_ADC_MODE_REQUEST_ADC_RESTART)
		{
			runADCMode = RUN_ADC_MODE_RUNNING;

			dma_hw->ints0 = 1u << adc_dma; // reset adc interrupt flag
			dma_channel_set_write_addr(adc_dma, ADC_Buffer[dmaPhase], true); // start writing into new buffer
			dma_channel_set_read_addr(spi_dma, SPI_Buffer[dmaPhase], true); // start reading from new buffer

			adc_set_round_robin(0);
			adc_select_input(0);
			adc_set_round_robin(0b0001111U);
			adc_run(true);
		}
		else if (runADCMode == RUN_ADC_MODE_ADC_STOPPED)
		{
			break;
		}
		   

	}
}

template<uint O, uint F> void CC<O, F>::abort()
{
	runADCMode = RUN_ADC_MODE_REQUEST_ADC_STOP;
}

	  

// Per-audio-sample ISR, called when two sets of ADC samples have been collected from all four inputs
template <uint OVERSAMPLE_BITS, uint F> __attribute__((section(".time_critical." "cc-buffer-full")))
void CC<OVERSAMPLE_BITS, F>::BufferFull()
{
	static int startupCounter = 8; // Decreases by 1 each sample, can do startup things when nonzero.
	static int mux_state = 0;
	static int norm_probe_count = 0;

	// Internal variables for IIR filters on knobs/cv
	static volatile int32_t knobssm[4] = { 0, 0, 0, 0 };
	static volatile int32_t cvsm[2] = { 0, 0 };
	__attribute__((unused)) static int np = 0, np1 = 0, np2 = 0;

	adc_select_input(0);

	// Advance external mux to next state
	int next_mux_state = (mux_state + 1) & 0x3;
	gpio_put(MX_A, next_mux_state & 1);
	gpio_put(MX_B, next_mux_state & 2);

	// Set up new writes into next buffer
	uint8_t cpuPhase = dmaPhase;
	dmaPhase = 1 - dmaPhase;

	dma_hw->ints0 = 1u << adc_dma; // reset adc interrupt flag
	dma_channel_set_write_addr(adc_dma, ADC_Buffer[dmaPhase], true); // start writing into new buffer
	dma_channel_set_read_addr(spi_dma, SPI_Buffer[dmaPhase], true); // start reading from new buffer

	////////////////////////////////////////
	// Collect various inputs and put them in variables for the DSP

	// Set CV inputs, with ~240Hz LPF on CV input
	int cvi = mux_state % 2;

	cvsm[cvi] = (15 * (cvsm[cvi]) + 16 * fix_dnl(ADC_Buffer[cpuPhase][3])) >> 4;
	cv[cvi] = 2048 - (cvsm[cvi] >> 4);


	// Set audio inputs, by averaging the two samples collected.
	// Invert to counteract inverting op-amp input configuration
	adcInR = 0;
	for (uint i = 0; i < OVERSAMPLES; ++i) adcInR += fix_dnl(ADC_Buffer[cpuPhase][0 + 4 * i]);
	adcInR = -((adcInR >> OVERSAMPLE_BITS) - 0x800);
 	adcInL = 0;
	for (uint i = 0; i < OVERSAMPLES; ++i) adcInL += fix_dnl(ADC_Buffer[cpuPhase][1 + 4 * i]);
	adcInL = -((adcInL >> OVERSAMPLE_BITS) - 0x800);

	// Set pulse inputs
	last_pulse[0] = pulse[0];
	last_pulse[1] = pulse[1];
	pulse[0] = !gpio_get(PULSE_1_INPUT);
	pulse[1] = !gpio_get(PULSE_2_INPUT);

	// Set knobs, with ~60Hz LPF
	int knob = mux_state;
	knobssm[knob] = (127 * (knobssm[knob]) + 16 * fix_dnl(ADC_Buffer[cpuPhase][2] >> 4)) >> 7;
	knobs[knob] = knobssm[knob];

	// Set switch value
	switch_ = static_cast<Switch>((knobs[3]>1000) + (knobs[3]>3000));
	if (startupCounter)
	{
		// Don't detect switch changes in first few cycles
		prev_switch = switch_;
		// Should initialise knob and CV smoothing filters here too
	}
	
	////////////////////////////
	// Normalisation probe

	if (use_norm_probe)
	{
		// Set normalisation probe output value
		// and update np to the expected history string
		if (norm_probe_count == 0)
		{
			int32_t normprobe = next_norm_probe();
			gpio_put(NORMALISATION_PROBE, normprobe);
			np = (np<<1)+(normprobe&0x1);
		}

		// CV sampled at 24kHz comes in over two successive samples
		if (norm_probe_count == 14 || norm_probe_count == 15)
		{
			plug_state[2+cvi] = (plug_state[2+cvi]<<1)+(ADC_Buffer[cpuPhase][3]<1800);
		}

		// Audio and pulse measured every sample at 48kHz
		if (norm_probe_count == 15)
		{
			plug_state[Input::Audio1] = (plug_state[Input::Audio1]<<1)+(ADC_Buffer[cpuPhase][1]<1800);
			plug_state[Input::Audio2] = (plug_state[Input::Audio2]<<1)+(ADC_Buffer[cpuPhase][0]<1800);
			plug_state[Input::Pulse1] = (plug_state[Input::Pulse1]<<1)+(pulse[0]);
			plug_state[Input::Pulse2] = (plug_state[Input::Pulse2]<<1)+(pulse[1]);

			for (int i=0; i<6; i++)
			{
				connected[i] = (np != plug_state[i]);
			}
		}
		
		// Force disconnected values to zero, rather than the normalisation probe garbage
		if (! is_connected(Input::Audio1)) adcInL = 0;
		if (! is_connected(Input::Audio2)) adcInR = 0;
		if (! is_connected(Input::CV1)) cv[0] = 0;
		if (! is_connected(Input::CV2)) cv[1] = 0;
		if (! is_connected(Input::Pulse1)) pulse[0] = 0;
		if (! is_connected(Input::Pulse2)) pulse[1] = 0;
	}
	
	////////////////////////////////////////
	// Run the DSP
	per_sample(*this);

	count++;

	////////////////////////////////////////
	// Collect DSP outputs and put them in the DAC SPI buffer
	// CV/Pulse outputs are done immediately in ProcessSample

	// Invert dacout to counteract inverting output configuration
	SPI_Buffer[cpuPhase][0] = dacval(-dac[0], DAC_CHANNEL_A);
	SPI_Buffer[cpuPhase][1] = dacval(-dac[1], DAC_CHANNEL_B);

	mux_state = next_mux_state;

	// If Abort called, stop ADC and DMA
	if (runADCMode == RUN_ADC_MODE_REQUEST_ADC_STOP)
	{
		adc_run(false);
		adc_set_round_robin(0);
		adc_select_input(0);

		dma_hw->ints0 = 1u << adc_dma; // reset adc interrupt flag
		dma_channel_cleanup(adc_dma);
		dma_channel_cleanup(spi_dma);
		irq_remove_handler(DMA_IRQ_0, CC::AudioCallback);
		
		runADCMode = RUN_ADC_MODE_ADC_STOPPED;
	}

	norm_probe_count = (norm_probe_count + 1) & 0xF;

	prev_switch = switch_;
	
	if (startupCounter) startupCounter--;
}

template<uint O, uint F> CC<O, F>::HardwareVersion CC<O, F>::ProbeHardwareVersion()
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
		return static_cast<CC::HardwareVersion>(id);
	default:
		return Unknown;
	}
}

template <uint O, uint F> CC<O, F>::CC()
{
		
	runADCMode = RUN_ADC_MODE_RUNNING;

	adc_run(false);
	adc_select_input(0);


	use_norm_probe = false;
	for (int i=0; i<6; i++)
	{
		connected[i] = false;
	}


	// Board version ID pins
	gpio_init(BOARD_ID_0);
	gpio_init(BOARD_ID_1);
	gpio_init(BOARD_ID_2);
	gpio_set_dir(BOARD_ID_0, GPIO_IN);
	gpio_set_dir(BOARD_ID_1, GPIO_IN);
	gpio_set_dir(BOARD_ID_2, GPIO_IN);
	hw = ProbeHardwareVersion();
	
	// USB host status pin
	gpio_init(USB_HOST_STATUS);
	gpio_disable_pulls(USB_HOST_STATUS);

	// Normalisation probe pin
	gpio_init(NORMALISATION_PROBE);
	gpio_set_dir(NORMALISATION_PROBE, GPIO_OUT);
	gpio_put(NORMALISATION_PROBE, false);

	adc_init(); // Initialize the ADC

	// Set ADC pins
	adc_gpio_init(AUDIO_L_IN_1);
	adc_gpio_init(AUDIO_R_IN_1);
	adc_gpio_init(MUX_IO_1);
	adc_gpio_init(MUX_IO_2);

	// Initialize Mux Control pins
	gpio_init(MX_A);
	gpio_init(MX_B);
	gpio_set_dir(MX_A, GPIO_OUT);
	gpio_set_dir(MX_B, GPIO_OUT);

	// Initialize pulse out
	gpio_init(PULSE_1_RAW_OUT);
	gpio_set_dir(PULSE_1_RAW_OUT, GPIO_OUT);
	gpio_put(PULSE_1_RAW_OUT, true); // set raw value high (output low)

	gpio_init(PULSE_2_RAW_OUT);
	gpio_set_dir(PULSE_2_RAW_OUT, GPIO_OUT);
	gpio_put(PULSE_2_RAW_OUT, true); // set raw value high (output low)

	// Initialize pulse in
	gpio_init(PULSE_1_INPUT);
	gpio_set_dir(PULSE_1_INPUT, GPIO_IN);
	gpio_pull_up(PULSE_1_INPUT); // NB Needs pullup to activate transistor on inputs

	gpio_init(PULSE_2_INPUT);
	gpio_set_dir(PULSE_2_INPUT, GPIO_IN);
	gpio_pull_up(PULSE_2_INPUT); // NB: Needs pullup to activate transistor on inputs


	// Setup SPI for DAC output
	spi_init(SPI_PORT, 15625000);
	spi_set_format(SPI_PORT, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
	gpio_set_function(DAC_SCK, GPIO_FUNC_SPI);
	gpio_set_function(DAC_TX, GPIO_FUNC_SPI);
	gpio_set_function(DAC_CS, GPIO_FUNC_SPI);

	// Setup I2C for EEPROM
	i2c_init(i2c0, 100 * 1000);
	gpio_set_function(EEPROM_SDA, GPIO_FUNC_I2C);
	gpio_set_function(EEPROM_SCL, GPIO_FUNC_I2C);



	// Setup CV PWM
	// First, tell the CV pins that the PWM is in charge of the value.
	gpio_set_function(CV_OUT_1, GPIO_FUNC_PWM);
	gpio_set_function(CV_OUT_2, GPIO_FUNC_PWM);

	// now create PWM config struct
	pwm_config config = pwm_get_default_config();
	pwm_config_set_wrap(&config, 2047); // 11-bit PWM


	// now set this PWM config to apply to the two outputs
	// NB: CV_A and CV_B share the same PWM slice, which means that they share a PWM config
	// They have separate 'gpio_level's (output compare unit) though, so they can have different PWM on-times
	pwm_init(pwm_gpio_to_slice_num(CV_OUT_1), &config, true); // Slice 1, channel A
	pwm_init(pwm_gpio_to_slice_num(CV_OUT_2), &config, true); // slice 1 channel B (redundant to set up again)

	// set initial level to half way (0V)
	pwm_set_gpio_level(CV_OUT_1, 1024);
	pwm_set_gpio_level(CV_OUT_2, 1024);

// If not using UART pins for UART, instead use as debug lines
#ifndef ENABLE_UART_DEBUGGING
	// Debug pins
	gpio_init(DEBUG_1);
	gpio_set_dir(DEBUG_1, GPIO_OUT);

	gpio_init(DEBUG_2);
	gpio_set_dir(DEBUG_2, GPIO_OUT);
#endif

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



// Read a byte from EEPROM
template<uint O, uint F> uint8_t CC<O, F>::read_byte_from_eeprom(unsigned int eeAddress)
{
	uint8_t deviceAddress = EEPROM_PAGE_ADDRESS | ((eeAddress >> 8) & 0x0F);
	uint8_t data = 0xFF;

	uint8_t addr_low_byte = eeAddress & 0xFF;
	i2c_write_blocking(i2c0, deviceAddress, &addr_low_byte, 1, false);

	i2c_read_blocking(i2c0, deviceAddress, &data, 1, false);
	return data;
}

// Read a 16-bit integer from EEPROM
template<uint O, uint F> int CC<O, F>::read_int_from_eeprom(unsigned int eeAddress)
{
	uint8_t highByte = read_byte_from_eeprom(eeAddress);
	uint8_t lowByte = read_byte_from_eeprom(eeAddress + 1);
	return (highByte << 8) | lowByte;
}

template<uint O, uint F> uint16_t CC<O, F>::crc_encode(const uint8_t *data, uint length)
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


template<uint O, uint F> int CC<O, F>::read_eeprom()
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
	for (int i = 0; i < EEPROM_NUM_BYTES; i++)
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

template<uint O, uint F> void CC<O, F>::calc_cal_coeffs(uint channel)
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


template<uint O, uint F> uint32_t CC<O, F>::midi_to_dac(int midiNote, int channel) {
	int32_t dacValue = ((cal_coeffs[channel].mi * (midiNote - 60)) >> 4) + cal_coeffs[channel].bi;
	if (dacValue > 524287) dacValue = 524287;
	if (dacValue < 0) dacValue = 0;
	return dacValue;
}

#endif
