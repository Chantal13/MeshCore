#pragma once

// ─── T-LoRa Pager Target Header ───────────────────────────────────────────
// MCU  : ESP32-S3 (16 MB Flash, 8 MB PSRAM)
// Radio: SX1262 (primary) or LR1121 (alternate variant)
// Board: LILYGO T-LoRa Pager

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <helpers/radiolib/RadioLibWrappers.h>
#include <helpers/radiolib/CustomSX1262Wrapper.h>
#include <helpers/ESP32Board.h>
#include <helpers/AutoDiscoverRTCClock.h>
#include <helpers/SensorManager.h>
#include <helpers/sensors/MicroNMEALocationProvider.h>
#include <helpers/sensors/EnvironmentSensorManager.h>

#ifdef DISPLAY_CLASS
  #include "TLoRaPagerDisplay.h"
  #include <helpers/ui/MomentaryButton.h>
#endif

// TLoRaPagerBoard pre-initialises the LoRa SPI bus (SPI2/FSPI) inside begin(),
// which main.cpp calls before display.begin().  This ensures SPI2_HOST claims
// GPIO34/33/35 first; when LGFX later calls spi_bus_initialize(SPI2_HOST, …) it
// receives ESP_ERR_INVALID_STATE (bus already up) and handles it gracefully by
// proceeding to spi_bus_add_device() — no pin-reservation conflict.
// begin() body is in target.cpp (where lora_spi is visible).
class TLoRaPagerBoard : public ESP32Board {
public:
  void begin();
};

extern TLoRaPagerBoard board;
extern WRAPPER_CLASS radio_driver;
extern AutoDiscoverRTCClock rtc_clock;
extern EnvironmentSensorManager sensors;

#ifdef DISPLAY_CLASS
  extern TLoRaPagerDisplay display;
  extern MomentaryButton   user_btn;
#endif

bool     radio_init();
uint32_t radio_get_rng_seed();
void     radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr);
void     radio_set_tx_power(int8_t dbm);
mesh::LocalIdentity radio_new_identity();
