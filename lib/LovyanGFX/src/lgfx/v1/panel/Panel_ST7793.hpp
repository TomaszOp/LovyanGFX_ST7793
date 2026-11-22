/*----------------------------------------------------------------------------/
  Lovyan GFX - Graphics library for embedded devices.

Original Source:
 https://github.com/lovyan03/LovyanGFX/

Licence:
 [FreeBSD](https://github.com/lovyan03/LovyanGFX/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)

Contributors:
 [ciniml](https://github.com/ciniml)
 [mongonta0716](https://github.com/mongonta0716)
 [tobozo](https://github.com/tobozo)
/----------------------------------------------------------------------------*/
#pragma once

#include "Panel_LCD.hpp"

namespace lgfx
{
 inline namespace v1
 {
//----------------------------------------------------------------------------

  // Sitronix ST7793 (240x400). 16-bit register index (R000h~R7FFh),
  // on an 8-bit bus every index / register value is sent upper byte first.
  struct Panel_ST7793 : public Panel_LCD
  {
    Panel_ST7793(void)
    {
      _cfg.memory_width  = _cfg.panel_width  = 240;
      _cfg.memory_height = _cfg.panel_height = 400;
      _cfg.readable = false;
      // GRAM read: the first word after the address set is invalid (datasheet 10.2.16).
      _cfg.dummy_read_pixel = 16;
      _cfg.dummy_read_bits  = 0;
      // A lone 8bit NOP would break the upper/lower byte pairing of the index register.
      _nop_closing = false;
    }

    bool init(bool use_reset) override;

    void setInvert(bool invert) override;
    void setSleep(bool flg) override;
    void setPowerSave(bool flg) override;

    void setWindow(uint_fast16_t xs, uint_fast16_t ys, uint_fast16_t xe, uint_fast16_t ye) override;
    color_depth_t setColorDepth(color_depth_t depth) override;

    void setRotation(uint_fast8_t r) override;

    void writeRegister(uint16_t reg, uint16_t data);
    uint16_t readRegister(uint16_t reg);

    // cmd = 16-bit register index (e.g. 0x0000 = device ID 7793h).
    uint32_t readCommand(uint_fast16_t cmd, uint_fast8_t index = 0, uint_fast8_t length = 4) override;
    void readRect(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, void* dst, pixelcopy_t* param) override;

  protected:

    void update_madctl(void) override;

    static constexpr uint16_t REG_OUTPUT_CTRL   = 0x0001; // SS, SM
    static constexpr uint16_t REG_ENTRY_MODE    = 0x0003; // BGR, ORG, I/D, AM
    static constexpr uint16_t REG_DISPLAY_CTRL1 = 0x0007; // BASEE
    static constexpr uint16_t REG_DISPLAY_CTRL2 = 0x0008; // FP, BP
    static constexpr uint16_t REG_EIGHT_COLOR   = 0x000B; // COL
    static constexpr uint16_t REG_POWER_CTRL1   = 0x0100; // DSTB
    static constexpr uint16_t REG_POWER_CTRL3   = 0x0102; // PSON, PON
    static constexpr uint16_t REG_RAM_ADDR_H    = 0x0200;
    static constexpr uint16_t REG_RAM_ADDR_V    = 0x0201;
    static constexpr uint16_t REG_RAMWR         = 0x0202;
    static constexpr uint16_t REG_H_START       = 0x0210;
    static constexpr uint16_t REG_H_END         = 0x0211;
    static constexpr uint16_t REG_V_START       = 0x0212;
    static constexpr uint16_t REG_V_END         = 0x0213;
    static constexpr uint16_t REG_BASE_IMAGE1   = 0x0400; // GS, NL, SCN
    static constexpr uint16_t REG_BASE_IMAGE2   = 0x0401; // NDL, VLE, REV
    static constexpr uint16_t REG_BASE_IMAGE3   = 0x0404; // VL

    void write_reg_index(uint16_t reg);

    void setColorDepth_impl(color_depth_t depth) override { (void)depth; _write_depth = rgb565_2Byte; _read_depth = rgb565_2Byte; }
  };

//----------------------------------------------------------------------------
 }
}
