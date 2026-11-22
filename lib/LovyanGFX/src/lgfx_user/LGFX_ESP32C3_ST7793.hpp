#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#if !defined (CONFIG_IDF_TARGET_ESP32C3)
  #error "This LGFX configuration is for ESP32-C3 only"
#endif

// 3.6" TFT LCD Shield, ST7793, 240x400, 8-bit 8080 parallel.
//
// ESP32-C3 wiring (GPIO 11-17 = flash, 18/19 = USB, 20/21 = UART0/Serial).
// GPIO2, GPIO8 and GPIO9 are strapping pins. The shield drives them only through
// its buffer inputs (high impedance), so boot is not affected.
//
//   shield      ESP32-C3
//   LCD_D0..D7  GPIO0..GPIO7
//   LCD_WR      GPIO10
//   LCD_RS      GPIO9   (BOOT button / strap pin, output after boot)
//   LCD_RST     GPIO8   (onboard RGB LED / strap pin)
//   LCD_CS      GND     (the panel is the only device on the bus)
//   LCD_RD      3V3     (write only, no free GPIO left)
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
      cfg.pin_wr = 10;
      cfg.pin_rd = -1;
      cfg.pin_rs = 9;
      cfg.pin_d0 = 0;
      cfg.pin_d1 = 1;
      cfg.pin_d2 = 2;
      cfg.pin_d3 = 3;
      cfg.pin_d4 = 4;
      cfg.pin_d5 = 5;
      cfg.pin_d6 = 6;
      cfg.pin_d7 = 7;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs   = -1;
      cfg.pin_rst  = 8;
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
