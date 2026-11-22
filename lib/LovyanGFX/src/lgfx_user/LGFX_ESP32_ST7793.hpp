#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#if !defined (CONFIG_IDF_TARGET_ESP32)
  #error "This LGFX configuration is for ESP32 only"
#endif

// 3.6" TFT LCD Shield, ST7793, 240x400, 8-bit 8080 parallel.
//
// ESP32 wiring (Bus_Parallel8 uses I2S; avoids GPIO 6-11 = flash, 34-39 = input only,
// 0/2/5/12/15 = strap, 1/3 = UART0, 16/17 = PSRAM on WROVER).
//
//   shield      ESP32
//   LCD_D0      GPIO13
//   LCD_D1      GPIO14
//   LCD_D2      GPIO18
//   LCD_D3      GPIO19
//   LCD_D4      GPIO21
//   LCD_D5      GPIO22
//   LCD_D6      GPIO23
//   LCD_D7      GPIO25
//   LCD_WR      GPIO26
//   LCD_RS      GPIO27
//   LCD_RD      GPIO32
//   LCD_CS      GPIO33
//   LCD_RST     GPIO4
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
      cfg.freq_read  =  1000000; // read cycle >= 450ns, RD low >= 170ns, RD high >= 250ns
      cfg.pin_wr = 26;
      cfg.pin_rd = 32;
      cfg.pin_rs = 27;
      cfg.pin_d0 = 13;
      cfg.pin_d1 = 14;
      cfg.pin_d2 = 18;
      cfg.pin_d3 = 19;
      cfg.pin_d4 = 21;
      cfg.pin_d5 = 22;
      cfg.pin_d6 = 23;
      cfg.pin_d7 = 25;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs   = 33;
      cfg.pin_rst  = 4;
      cfg.pin_busy = -1;

      cfg.panel_width      = 240;
      cfg.panel_height     = 400;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;
      // Reading requires bidirectional buffers on the shield powered from 3.3V
      // (5V buffers would drive 5V into the ESP32 data pins during a read).
      cfg.readable         = true;  // verified on esp32dev (ID, registers, readPixel)
      cfg.dummy_read_pixel = 16;    // one dummy word (2 bytes) before GRAM data, verified
      cfg.invert           = false;
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = false;
      _panel_instance.config(cfg);
    }
    setPanel(&_panel_instance);
  }
};
