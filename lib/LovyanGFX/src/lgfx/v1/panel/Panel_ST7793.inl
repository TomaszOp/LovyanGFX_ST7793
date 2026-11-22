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
#ifndef LGFX_V1_IMPLEMENTATION
#error "Panel_ST7793.inl is part of lgfx_v1.cpp and is not meant to be included on its own"
#endif
#include "Panel_ST7793.hpp"

#include "../misc/colortype.hpp"
#include "../Bus.hpp"
#include "../platforms/common.hpp"

namespace lgfx
{
 inline namespace v1
 {
//----------------------------------------------------------------------------

  void Panel_ST7793::write_reg_index(uint16_t reg)
  {
    _bus->writeCommand(getSwap16(reg), 16);
  }

  void Panel_ST7793::writeRegister(uint16_t reg, uint16_t data)
  {
    write_reg_index(reg);
    _bus->writeData(getSwap16(data), 16);
  }

  uint32_t Panel_ST7793::readCommand(uint_fast16_t cmd, uint_fast8_t index, uint_fast8_t length)
  {
    startWrite();
    write_reg_index(cmd);
    _bus->beginRead((index << 3) + _cfg.dummy_read_bits);

    uint32_t res = 0;
    for (size_t i = 0; i < length; ++i)
    {
      res += (_bus->readData(8) & 0xFF) << (i * 8);
    }
    cs_control(true);
    _bus->endRead();
    endWrite();

    if (_in_transaction) { cs_control(false); }
    return res;
  }

  uint16_t Panel_ST7793::readRegister(uint16_t reg)
  {
    // Upper byte is transferred first.
    return getSwap16(readCommand(reg, 0, 2));
  }

  void Panel_ST7793::readRect(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, void* dst, pixelcopy_t* param)
  {
    uint_fast16_t bytes = param->dst_bits >> 3;
    auto len = w * h;
    if (!_cfg.readable)
    {
      memset(dst, 0, len * bytes);
      return;
    }

    startWrite();
    // setWindow ends with the R202h index, which is also the GRAM read register.
    setWindow(x, y, x + w - 1, y + h - 1);
    // In 8-bit mode each pixel is read as 2 bytes (RGB565, upper byte first).
    _bus->beginRead(_cfg.dummy_read_pixel);

    if (param->no_convert)
    {
      _bus->readBytes((uint8_t*)dst, len * bytes, false);
    }
    else
    {
      _bus->readPixels(dst, param, len);
    }
    cs_control(true);
    _bus->endRead();

    endWrite();

    if (_in_transaction) { cs_control(false); }
  }

  bool Panel_ST7793::init(bool use_reset)
  {
    // Panel_LCD::init performs the hardware reset (>1ms required, see datasheet Fig.55);
    // getInitCommands() returns nullptr, so no 8bit command list is sent there.
    if (!Panel_LCD::init(use_reset))
    {
      return false;
    }

    startWrite(true);

    // Transfer synchronization of the 8bit interface : write 00h with RS=0 four times.
    _bus->writeCommand(0, 32);

    // Gamma / VCOM / source & gate voltages are loaded from NVM on reset.
    writeRegister(REG_OUTPUT_CTRL  , 0x0100); // SS=1 (S720->S1), SM=0
    writeRegister(REG_DISPLAY_CTRL2, 0x0808); // FP=8, BP=8
    writeRegister(REG_EIGHT_COLOR  , 0x0030); // COL=0 (262k color)
    writeRegister(REG_BASE_IMAGE1  , 0x6200); // GS=0, NL=31h (400 lines), SCN=0
    writeRegister(REG_BASE_IMAGE2  , 0x0001); // REV=1 (normal colors on this panel type)
    writeRegister(REG_BASE_IMAGE3  , 0x0000); // VL=0
    writeRegister(REG_POWER_CTRL3  , 0x01B0); // PSON=1, PON=1 : turn on internal power supply
    _bus->flush();
    delay(50);
    writeRegister(REG_DISPLAY_CTRL1, 0x0100); // BASEE=1 : display on
    _bus->flush();

    endWrite();

    return true;
  }

  void Panel_ST7793::setInvert(bool invert)
  {
    _invert = invert;
    startWrite();
    // REV=1 is the non-inverted state for this driver.
    writeRegister(REG_BASE_IMAGE2, (invert ^ _cfg.invert) ? 0x0000 : 0x0001);
    _bus->flush();
    endWrite();
  }

  void Panel_ST7793::setSleep(bool flg)
  {
    // Shutdown mode (DSTB) loses RAM and register contents, so sleep only turns
    // off the display and the internal LCD power supply (datasheet Fig.56/57).
    startWrite();
    if (flg)
    {
      writeRegister(REG_DISPLAY_CTRL1, 0x0000); // BASEE=0
      writeRegister(REG_POWER_CTRL3  , 0x0180); // PSON=0, PON=0
      _bus->flush();
    }
    else
    {
      writeRegister(REG_POWER_CTRL3  , 0x01B0); // PSON=1, PON=1
      _bus->flush();
      delay(50);
      writeRegister(REG_DISPLAY_CTRL1, 0x0100); // BASEE=1
      _bus->flush();
    }
    endWrite();
  }

  void Panel_ST7793::setPowerSave(bool flg)
  {
    startWrite();
    writeRegister(REG_EIGHT_COLOR, flg ? 0x0031 : 0x0030); // COL : 8-color mode
    _bus->flush();
    endWrite();
  }

  color_depth_t Panel_ST7793::setColorDepth(color_depth_t depth)
  {
    setColorDepth_impl(depth);

    update_madctl();
    return _write_depth;
  }

  void Panel_ST7793::setRotation(uint_fast8_t r)
  {
    r &= 7;
    _rotation = r;
    _internal_rotation = ((r + _cfg.offset_rotation) & 3) | ((r & 4) ^ (_cfg.offset_rotation & 4));

    _width  = _cfg.panel_width;
    _height = _cfg.panel_height;
    if (_internal_rotation & 1) std::swap(_width, _height);

    _colstart = _cfg.offset_x;
    _rowstart = _cfg.offset_y;

    _xs = _xe = _ys = _ye = INT16_MAX;

    update_madctl();
  }

  void Panel_ST7793::update_madctl(void)
  {
    if (_bus == nullptr) return;

    // R003h : I/D1(D5)=vertical increment, I/D0(D4)=horizontal increment, AM(D3)=vertical write.
    uint32_t data = 0;
    switch (_internal_rotation)
    {
    case 0: data = 0x0030; break;
    case 1: data = 0x0028; break;
    case 2: data = 0x0000; break;
    case 3: data = 0x0018; break;
    case 4: data = 0x0010; break;
    case 5: data = 0x0038; break;
    case 6: data = 0x0020; break;
    case 7: data = 0x0008; break;
    }
    if (!_cfg.rgb_order) data |= 0x1000; // BGR

    startWrite();
    writeRegister(REG_ENTRY_MODE, data);
    _bus->flush();
    endWrite();
  }

  void Panel_ST7793::setWindow(uint_fast16_t xs, uint_fast16_t ys, uint_fast16_t xe, uint_fast16_t ye)
  {
    if (_internal_rotation & 1)
    {
      std::swap(xs, ys);
      std::swap(xe, ye);
    }

    // Convert to physical RAM coordinates. With ORG=0 the address counter must be
    // set to the window corner where writing starts (depends on I/D).
    uint_fast8_t rb = 1u << _internal_rotation;
    uint_fast16_t ax = xs;
    uint_fast16_t ay = ys;
    if (rb & 0b11000110) // case 1:2:6:7: horizontal decrement
    {
      auto tmp = xs;
      xs = _cfg.panel_width - xe - 1;
      xe = _cfg.panel_width - tmp - 1;
      ax = xe;
    }
    else
    {
      ax = xs;
    }
    if (rb & 0b10011100) // case 2:3:4:7: vertical decrement
    {
      auto tmp = ys;
      ys = _cfg.panel_height - ye - 1;
      ye = _cfg.panel_height - tmp - 1;
      ay = ye;
    }
    else
    {
      ay = ys;
    }

    xs += _colstart; xe += _colstart; ax += _colstart;
    ys += _rowstart; ye += _rowstart; ay += _rowstart;

    if (xs != _xs || xe != _xe)
    {
      _xs = xs;
      _xe = xe;
      writeRegister(REG_H_START, xs);
      writeRegister(REG_H_END  , xe);
    }
    if (ys != _ys || ye != _ye)
    {
      _ys = ys;
      _ye = ye;
      writeRegister(REG_V_START, ys);
      writeRegister(REG_V_END  , ye);
    }
    // The address counter is always reloaded, a previous write may have stopped mid-window.
    writeRegister(REG_RAM_ADDR_H, ax);
    writeRegister(REG_RAM_ADDR_V, ay);
    write_reg_index(REG_RAMWR);
  }

//----------------------------------------------------------------------------
 }
}
