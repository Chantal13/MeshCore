#include <Arduino.h>
#include "target.h"

// ─── Board ────────────────────────────────────────────────────────────────
// TLoRaPagerBoard::begin() pre-initialises lora_spi (SPI2/FSPI) BEFORE
// display.begin() runs in main.cpp.  In ESP-IDF 5.x, spi_bus_initialize()
// calls esp_gpio_reserve_pins() for the supplied GPIO pins.  If LGFX ran
// spi_bus_initialize(SPI3_HOST, {34,33,35}) first, those pins would be
// reserved for SPI3 and the subsequent spi_bus_initialize(SPI2_HOST, {34,33,35})
// inside lora_spi.begin() would fail, leaving RadioLib unable to communicate
// with the SX1262 (radio_init() returns false → halt()).
//
// By calling lora_spi.begin() here (in board.begin(), which main.cpp calls
// before display.begin()), SPI2_HOST claims the pins first.  LGFX is set to
// SPI2_HOST with bus_shared=true; when it calls spi_bus_initialize(SPI2_HOST,…)
// it receives ESP_ERR_INVALID_STATE and handles it gracefully, proceeding to
// spi_bus_add_device() on the already-live bus.
void TLoRaPagerBoard::begin() {
  ESP32Board::begin();
  lora_spi.begin(P_LORA_SCLK, P_LORA_MISO, P_LORA_MOSI);
}

TLoRaPagerBoard board;

// ─── Radio ────────────────────────────────────────────────────────────────
// SPI bus is shared between LoRa (CS=IO36), NFC (CS=IO39), and Display (CS=IO38).
// Both RadioLib and LGFX use SPI2_HOST (FSPI) on GPIO34/33/35.
// lora_spi is pre-initialised in TLoRaPagerBoard::begin() before display.begin().
SPIClass lora_spi;  // SPI2 (FSPI); pre-initialised in board.begin()

RADIO_CLASS radio = new Module(
  P_LORA_NSS,    // CS   – IO36
  P_LORA_DIO_1,  // DIO1 – IO14
  P_LORA_RESET,  // RESET– IO47
  P_LORA_BUSY,   // BUSY – IO48
  lora_spi
);

WRAPPER_CLASS radio_driver(radio, board);

// ─── Clocks ───────────────────────────────────────────────────────────────
ESP32RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);

// ─── GPS + Sensors ────────────────────────────────────────────────────────
// MIA-M10Q: ESP32-S3 receives on IO12 (GPS TX), transmits on IO4 (GPS RX).
// sensors is always required by MyMesh / UITask regardless of GPS hardware.
MicroNMEALocationProvider gps(Serial1, &rtc_clock);
EnvironmentSensorManager sensors(gps);

// ─── Display & User Input ─────────────────────────────────────────────────
// Rotary encoder centre button (IO7) used as the user button.
#ifdef DISPLAY_CLASS
  TLoRaPagerDisplay display;
  MomentaryButton   user_btn(PIN_USER_BTN, 1000, true);  // active-low
#endif

// ─── radio_init ───────────────────────────────────────────────────────────
bool radio_init() {
  fallback_clock.begin();

  // I2C bus: SDA=IO3, SCL=IO2
  // Shared by: BQ27220 gauge, BQ25896 charger, DRV2605 haptic, BHI260 IMU,
  //            RTC, keyboard controller.
  // Wire.begin() was already called by ESP32Board::begin() inside board.begin();
  // this call to rtc_clock.begin() is still needed to probe for an I2C RTC.
  rtc_clock.begin(Wire);

  // GPS UART is initialised by EnvironmentSensorManager::initBasicGPS()
  // using PIN_GPS_TX=4 and PIN_GPS_RX=12 defined in platformio.ini.

  // lora_spi was pre-initialised in TLoRaPagerBoard::begin(); std_init() calls
  // lora_spi.begin() again but SPIClass detects _spi != NULL and returns early,
  // so the existing SPI2_HOST bus (and its GPIO routing) is preserved.
  return radio.std_init(&lora_spi);
}

uint32_t radio_get_rng_seed() {
  return radio.random(0x7FFFFFFF);
}

void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr) {
  radio.setFrequency(freq);
  radio.setSpreadingFactor(sf);
  radio.setBandwidth(bw);
  radio.setCodingRate(cr);
}

void radio_set_tx_power(int8_t dbm) {
  radio.setOutputPower(dbm);
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);
}
