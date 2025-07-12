
#include "weas/codec.h"

#include <functional>

#include "cosas/maths.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/structs/io_bank0.h"
#include "hardware/structs/spi.h"
#include "weas/led.h"


void callback() {
    Codec::get().adc_callback();
}

Codec::Codec() {

  for (uint lr = 0; lr < 2; lr++) {
    gpio_init(PLS_OUT + lr);
    gpio_set_dir(PLS_OUT + lr, GPIO_OUT);
    gpio_put(PLS_OUT + lr, true);
    gpio_init(PLS_IN + lr);
    gpio_set_dir(PLS_IN+ lr, GPIO_IN);
    gpio_pull_up(PLS_IN + lr);
  }

  spi_init(spi0, 15625000);  // why this baudrate?  max is 20MHz
  spi_set_format(spi0, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  gpio_set_function(DAC_SCK, GPIO_FUNC_SPI);
  gpio_set_function(DAC_TX, GPIO_FUNC_SPI);
  gpio_set_function(DAC_CS, GPIO_FUNC_SPI);
  dac_dma = dma_claim_unused_channel(true);
  dma_channel_config spi_dmacfg = dma_channel_get_default_config(dac_dma);
  channel_config_set_transfer_data_size(&spi_dmacfg, DMA_SIZE_16);
  channel_config_set_dreq(&spi_dmacfg, DREQ_SPI0_TX);
  dma_channel_configure(dac_dma, &spi_dmacfg, &spi_get_hw(spi0)->dr, NULL, 2, false);

  for (uint mux = 0; mux < 2; mux++) {
    gpio_init(MUX_OUT + mux);
    gpio_set_dir(MUX_OUT, GPIO_OUT);
    adc_gpio_init(MUX_IN + mux);
  }

  for (uint adc = 0; adc < 2; adc++) adc_gpio_init(ADC_IN + adc);
  adc_select_input(0);
  adc_set_round_robin(0b1111);  // audio and MUX
  adc_fifo_setup(true, true, 1, false, false);
  adc_set_clkdiv(48000000/(4 * OVERSAMPLE * SAMPLE_FREQ) - 1);
  adc_dma = dma_claim_unused_channel(true);
  dma_channel_config adc_dmacfg = dma_channel_get_default_config(adc_dma);
  channel_config_set_transfer_data_size(&adc_dmacfg, DMA_SIZE_16);
  channel_config_set_read_increment(&adc_dmacfg, false);
  channel_config_set_write_increment(&adc_dmacfg, true);
  channel_config_set_dreq(&adc_dmacfg, DREQ_ADC);
  dma_channel_configure(adc_dma, &adc_dmacfg, adc_buffer[dma_phase], &adc_hw->fifo, 8, true);
  dma_channel_set_irq0_enabled(adc_dma, true);

  irq_set_enabled(DMA_IRQ_0, true);
  irq_set_exclusive_handler(DMA_IRQ_0, callback);
}

Codec& Codec::get() {
  static Codec instance;
  return instance;
}


void Codec::adc_callback() {

  static volatile int32_t knobs_smooth[4] = {0, 0, 0, 0};
  static volatile int32_t cv_smooth[2] = {0, 0};

  adc_select_input(0);  // is this needed here?
  cpu_phase = dma_phase;
  dma_phase = 1 - dma_phase;
  dma_channel_set_write_addr(adc_dma, adc_buffer[dma_phase], true);
  dma_channel_set_read_addr(dac_dma, spi_buffer[dma_phase], true);

  uint cv_idx = count % 2;
  // this almost fits in 16 bits, but i guess there's no advantage in pushing it to do so?
  cv_smooth[cv_idx] = (15 * (cv_smooth[cv_idx]) + 16 * fix_dnl(adc_buffer[cpu_phase][3])) >> 4;
  cv[cv_idx] = static_cast<int16_t>(2048 - cv_smooth[cv_idx]);  // cc has an extra >> 4 here i don't understand

  for (uint lr = 0; lr < 2; lr++) {
    adc[lr] = -(fix_dnl((adc_buffer[cpu_phase][0+lr] + adc_buffer[cpu_phase][4+lr]) >> 1) - 0x1000);
    pulse[1][lr] = pulse[0][lr];
    pulse[0][lr] = !gpio_get(PLS_IN);
  }

  const uint knob = count & 0x3;
  knobs_smooth[knob] = (127 * (knobs_smooth[knob]) + 16 * adc_buffer[cpu_phase][6]) >> 7;
  knobs[1][knob] = knobs[0][knob];
  knobs[0][knob] = knobs_smooth[knob];  // extra >> 4 here
  if (knob == 3) {
    switch_[1] = switch_[0];
    switch_[0] = static_cast<Switch>((knobs[0][knob]>1000) + (knobs[0][knob]>3000));
  }

  count++;
  for (uint mux = 0; mux < 2; mux++) gpio_put(MUX_OUT + mux, count & (0b1 << mux));
  dma_hw->ints0 = 1u << adc_dma;  // reset adc interrupt flag (we have handled it)
}
