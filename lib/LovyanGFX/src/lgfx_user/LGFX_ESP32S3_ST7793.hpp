#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#if !defined (CONFIG_IDF_TARGET_ESP32S3)
  #error "This LGFX configuration is for ESP32-S3 only"
#endif

// 3.6" TFT LCD Shield, ST7793, 240x400, 8-bit 8080 parallel.
//
// ESP32-S3 wiring (avoids GPIO 0/3/45/46 = strap, 19/20 = USB, 26-37 = flash/PSRAM,
// 43/44 = UART0, 47/48 = VDD_SPI domain / RGB LED).
//
//   shield      ESP32-S3
//   LCD_D0..D7  GPIO4..GPIO11
//   LCD_WR      GPIO12
//   LCD_RS      GPIO13
//   LCD_RD      GPIO14
//   LCD_CS      GPIO21
//   LCD_RST     GPIO1
//   5V / GND    5V / GND

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ST7793  _panel_instance;
  lgfx::Bus_Parallel8 _bus_instance;

public:
  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();
      cfg.freq_write = 10000000; // ST7793 write cycle >= 75ns
      cfg.pin_wr = 12;
      cfg.pin_rd = 14;
      cfg.pin_rs = 13;
      cfg.pin_d0 = 4;
      cfg.pin_d1 = 5;
      cfg.pin_d2 = 6;
      cfg.pin_d3 = 7;
      cfg.pin_d4 = 8;
      cfg.pin_d5 = 9;
      cfg.pin_d6 = 10;
      cfg.pin_d7 = 11;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs   = 21;
      cfg.pin_rst  = 1;
      cfg.pin_busy = -1;

      cfg.panel_width      = 240;
      cfg.panel_height     = 400;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;
      cfg.readable         = false; // shield buffers (U2/U3) may block reading
      cfg.invert           = false;
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = false;
      _panel_instance.config(cfg);
    }
    setPanel(&_panel_instance);
  }
};
