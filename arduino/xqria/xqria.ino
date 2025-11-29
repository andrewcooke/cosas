
#include <Arduino.h>
#include <array>
#include <numeric>
#include <mutex>
#include <thread>
#include "esp_timer.h"
#include "driver/dac_continuous.h"
#include "math.h"

const uint DMA_BUFFER_SIZE = 4092;  // 32 to 4092, multiple of 4; padded 16 bit; audible artefacts at 256 (even 1024) and below and i don't understand why
const uint MAX_LOCAL_BUFFER_SIZE = DMA_BUFFER_SIZE / 2;  // 8 bit, so half size of 16 bit
const uint MIN_LOCAL_BUFFER_SIZE = 10;  // anything lower grinds
const uint SAMPLE_RATE_HZ = 40000;
const uint LOWEST_F_HZ = 20;
const uint N_SAMPLES = SAMPLE_RATE_HZ / LOWEST_F_HZ;  // for completely sampled lowest freq (2,000 here)
const uint PHASE_EXTN = 2;  // extra bits for phase to allow better resolution at low f
const uint N_SAMPLES_EXTN = N_SAMPLES << PHASE_EXTN;

uint LOCAL_BUFFER_SIZE = MAX_LOCAL_BUFFER_SIZE;

const uint N12 = 1 << 12;
const uint MAX12 = N12 - 1;

template<typename T> int sgn(T val) {
  return (T(0) < val) - (val < T(0));
}

template<typename T> class EMA {
private:
  uint bits;
  uint denom;
  uint xtra;
  T state;
protected:
  virtual T get_state() {
    return state;
  }
  virtual void set_state(T s) {
    state = s;
  }
public:
  uint num;
  EMA(uint bits, uint num, uint xtra, T state)
    : bits(bits), denom(1 << bits), xtra(xtra), state(state << xtra), num(num){};
  T next(T val) {
    set_state((get_state() * static_cast<T>(denom - num) + (val << xtra) * static_cast<T>(num)) >> bits);
    return read();
  }
  T read() {
    return get_state() >> xtra;
  }
};

class Quarter {
private:
  virtual uint lookup(uint phase);
public:
  Quarter() = default;
  int operator()(uint amp, int phase) {
    int sign = 1;
    if (phase >= N_SAMPLES / 2) {
      phase = N_SAMPLES - phase;
      sign = -1;
    };
    if (phase >= N_SAMPLES / 4) phase = (N_SAMPLES / 2) - phase;
    return sign * ((amp * lookup(phase)) >> 12);
  }
};

class Sine : public Quarter {
private:
  std::array<uint, 1 + N_SAMPLES / 4> table;
public:
  Sine() : Quarter() {
    for (uint i = 0; i < 1 + N_SAMPLES / 4; i++) table[i] = MAX12 * sin(2 * PI * i / N_SAMPLES);
  }
  uint lookup(uint phase) override {
    return table[phase];
  }
};

Sine SINE;

class Square : public Quarter {
public:
  Square() : Quarter() {}
  uint lookup(uint phase) override {
    return MAX12;
  }
};

Square SQUARE;

class Triangle : public Quarter {
public:
  Triangle() : Quarter() {}
  uint lookup(uint phase) override {
    return (4 * MAX12 * phase) / N_SAMPLES;
  }
};

Triangle TRIANGLE;

int norm_phase(int phase) {
  while (phase < 0) phase += N_SAMPLES_EXTN;
  while (phase >= N_SAMPLES_EXTN) phase -= N_SAMPLES_EXTN;
  return phase;
}

static uint8_t BUFFER[MAX_LOCAL_BUFFER_SIZE];
static SemaphoreHandle_t dma_semaphore;
static BaseType_t dma_flag = pdFALSE;
static dac_continuous_handle_t dac_handle;
static uint DMA_ERRORS = 0;

// fast copy of existing data into buffer
static IRAM_ATTR bool dac_callback(dac_continuous_handle_t handle, const dac_event_data_t* event, void* user_data) {
  uint loaded = 0;
  dac_continuous_write_asynchronously(handle, static_cast<uint8_t*>(event->buf), event->buf_size, BUFFER, LOCAL_BUFFER_SIZE, &loaded);
  if (loaded != LOCAL_BUFFER_SIZE) DMA_ERRORS += 1;
  if (event->buf_size != DMA_BUFFER_SIZE) DMA_ERRORS += 1;
  if (event->write_bytes != 2 * LOCAL_BUFFER_SIZE) DMA_ERRORS += 1;
  xSemaphoreGiveFromISR(dma_semaphore, &dma_flag);  // flag refill
  return false;
}

// slow fill of buffer
void update_buffer() {
  static int phase = 0;
  static EMA<uint> ema = EMA<uint>(2, 1, 4, 0);
  int freq = ema.next(MAX12 - analogRead(13));
  int mn = 9999;
  int mx = -9999;
  int other = -1;
  for (uint i = 0; i < LOCAL_BUFFER_SIZE; i++) {
    // int sample = 128 + (SQUARE(MAX12, phase >> PHASE_EXTN) >> 5);
    // int sample = 128 + (TRIANGLE(MAX12, phase >> PHASE_EXTN) >> 5);
    int sample = 128 + (SINE(MAX12, phase >> PHASE_EXTN) >> 5);
    mx = max(mx, sample);
    mn = min(mn, sample);
    if (sample != mn && sample != mx) other = sample;
    BUFFER[i] = sample;
    phase = norm_phase(phase + freq);
  }
  Serial.printf("%d %d - %d (%d) %d\n", freq, mn, mx, other, DMA_ERRORS);
}

// refill buffer when flagged
void loop() {
  for (;;) {
    if (xSemaphoreTake(dma_semaphore, portMAX_DELAY) == pdTRUE) update_buffer();
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("hello world");
  pinMode(13, INPUT);
  update_buffer();  // first callback happens immediately
  dma_semaphore = xSemaphoreCreateBinary();
  dac_continuous_config_t dac_config = {
    .chan_mask = DAC_CHANNEL_MASK_CH0,
    .desc_num = 8,  // 2 allows pingpong for large buffers, but a larger value seems to help small buffers
    .buf_size = DMA_BUFFER_SIZE,
    .freq_hz = SAMPLE_RATE_HZ,
    .offset = 0,
    .clk_src = DAC_DIGI_CLK_SRC_DEFAULT,
    .chan_mode = DAC_CHANNEL_MODE_SIMUL  // not used for single channel
  };
  dac_continuous_new_channels(&dac_config, &dac_handle);
  dac_continuous_enable(dac_handle);
  dac_event_callbacks_t callbacks = {
    .on_convert_done = dac_callback,
    .on_stop = nullptr
  };
  dac_continuous_register_event_callback(dac_handle, &callbacks, nullptr);
  dac_continuous_start_async_writing(dac_handle);
};
