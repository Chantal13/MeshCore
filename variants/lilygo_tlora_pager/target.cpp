#include <Arduino.h>
#include "target.h"

// ─── Board ────────────────────────────────────────────────────────────────
ESP32Board board;

// ─── Radio ────────────────────────────────────────────────────────────────
// SPI bus is shared between LoRa (CS=IO36), NFC (CS=IO39), and Display (CS=IO38).
// RadioLib claims the default SPI (SPI2 / FSPI); LGFX uses SPI3 (HSPI).
static SPIClass lora_spi;  // default SPI2 (FSPI) for LoRa

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
  Wire.begin(PIN_BOARD_SDA, PIN_BOARD_SCL);
  rtc_clock.begin(Wire);

  // GPS UART is initialised by EnvironmentSensorManager::initBasicGPS()
  // using PIN_GPS_TX=4 and PIN_GPS_RX=12 defined in platformio.ini.

  // Initialise LoRa SPI bus then start the radio
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
