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

#ifdef ENABLE_GPS
  #include <helpers/sensors/MicroNMEALocationProvider.h>
  #include <helpers/sensors/EnvironmentSensorManager.h>
#endif

#ifdef DISPLAY_CLASS
  #include "TLoRaPagerDisplay.h"
  #include <helpers/ui/MomentaryButton.h>
#endif

extern ESP32Board board;
extern WRAPPER_CLASS radio_driver;
extern AutoDiscoverRTCClock rtc_clock;

#ifdef ENABLE_GPS
  extern EnvironmentSensorManager sensors;
#endif

#ifdef DISPLAY_CLASS
  extern TLoRaPagerDisplay display;
  extern MomentaryButton   user_btn;
#endif

bool     radio_init();
uint32_t radio_get_rng_seed();
void     radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr);
void     radio_set_tx_power(int8_t dbm);
mesh::LocalIdentity radio_new_identity();
