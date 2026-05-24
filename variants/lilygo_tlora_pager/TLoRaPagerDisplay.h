#pragma once

// LGFX display driver for the LILYGO T-LoRa Pager
// Display: 2.33" ST7796, 480x222 (landscape widescreen)
//
// SPI pins are SHARED between LoRa, NFC, and Display on this hardware.
// The display uses SPI3_HOST (HSPI) so it does not conflict with
// RadioLib which claims the default SPI (SPI2_HOST / FSPI).
//
// NOTE: The display CS pin is not exposed in the LILYGO pinout diagram;
//       it appears to be tied low on the PCB (display always selected).
//       Set PIN_TFT_CS below if your unit has an accessible CS pin.

#include <helpers/ui/LGFXDisplay.h>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class TLoRaPagerLGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7796 _panel;
  lgfx::Bus_SPI      _bus;
  lgfx::Light_PWM    _light;

public:
  TLoRaPagerLGFX() {
    // ── SPI bus ────────────────────────────────────────────────────────
    {
      auto cfg = _bus.config();
      cfg.spi_host   = SPI3_HOST;   // HSPI – separate from RadioLib's SPI2
      cfg.freq_write = 40000000;    // 40 MHz write
      cfg.freq_read  = 16000000;    // 16 MHz read
      cfg.pin_sclk   = 35;          // SCK  – shared with LoRa / NFC
      cfg.pin_mosi   = 34;          // MOSI – shared with LoRa / NFC
      cfg.pin_miso   = 33;          // MISO – shared with LoRa / NFC
      cfg.pin_dc     = 37;          // Data / Command select
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }

    // ── Panel ──────────────────────────────────────────────────────────
    // The ST7796 controller has 320 columns × 480 rows (portrait).
    // The physical LCD panel is 480 wide × 222 tall (widescreen landscape).
    // Configured in portrait here; LGFXDisplay::begin() calls setRotation(1)
    // which maps to: logical width=480, logical height=222.
    // The 222-pixel visible columns are centred in the 320-column controller
    // memory: offset_x = (320 − 222) / 2 = 49.
    {
      auto cfg = _panel.config();
      cfg.pin_cs          = -1;   // CS is tied low on PCB (not a GPIO)
      cfg.pin_rst         = -1;   // No dedicated reset pin
      cfg.pin_busy        = -1;
      cfg.memory_width    = 320;  // ST7796 native column count
      cfg.memory_height   = 480;  // ST7796 native row count
      cfg.panel_width     = 222;  // Physical visible columns (pre-rotation)
      cfg.panel_height    = 480;  // Physical visible rows (pre-rotation)
      cfg.offset_x        = 49;   // Centre 222-px panel in 320-px memory
      cfg.offset_y        = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable         = true;
      cfg.invert           = false;
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = false; // Display has its own SPI host (SPI3)
      _panel.config(cfg);
    }

    // ── Backlight ─────────────────────────────────────────────────────
    {
      auto cfg = _light.config();
      cfg.pin_bl      = 42;     // BL pin
      cfg.invert      = false;
      cfg.freq        = 44100;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }

    setPanel(&_panel);
  }
};

// Companion display class wrapping the LGFX device.
// UI_ZOOM=3 → logical canvas = 480/3 × 222/3 = 160 × 74 px
class TLoRaPagerDisplay : public LGFXDisplay {
  TLoRaPagerLGFX _disp;
public:
  TLoRaPagerDisplay() : LGFXDisplay(480, 222, _disp) {}
};
