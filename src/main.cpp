#include <sdkconfig.h>

#if defined (ARDUINO)
  #include <Arduino.h>
  #define LOG(...)      Serial.printf(__VA_ARGS__)
  #define FREE_HEAP()   ESP.getFreeHeap()
  #define YIELD_CPU()   do {} while (0)
#else
  // Pure ESP-IDF: console UART runs at 115200 from boot.
  #include <cstdio>
  #include <esp_heap_caps.h>
  #include <freertos/FreeRTOS.h>
  #include <freertos/task.h>
  #define LOG(...)      printf(__VA_ARGS__)
  #define FREE_HEAP()   heap_caps_get_free_size(MALLOC_CAP_8BIT)
  // Lets the IDLE task run, so the task watchdog does not complain during the animation.
  #define YIELD_CPU()   vTaskDelay(1)
#endif

#if defined (CONFIG_IDF_TARGET_ESP32C3)
  #include <lgfx_user/LGFX_ESP32C3_ST7793.hpp>
#elif defined (CONFIG_IDF_TARGET_ESP32S3)
  #include <lgfx_user/LGFX_ESP32S3_ST7793.hpp>
#elif defined (CONFIG_IDF_TARGET_ESP32)
  #include <lgfx_user/LGFX_ESP32_ST7793.hpp>
#else
  #error "No ST7793 LGFX configuration for this target"
#endif

#if !defined (ARDUINO)
  using lgfx::delay;
  using lgfx::millis;
#endif

// 1 = run the LCD read tests (ID, register readback, readPixel) at startup.
// 0 = no read access to the LCD at all.
#ifndef ST7793_READ_TEST
#define ST7793_READ_TEST 0
#endif

static LGFX lcd;

#if ST7793_READ_TEST
static lgfx::Panel_ST7793* panel(void)
{
  return static_cast<lgfx::Panel_ST7793*>(lcd.getPanel());
}

static bool readable(void)
{
  return panel()->config().readable;
}

// Reads the device ID (R000h). Prints the result on Serial and on the screen.
static void testReadID(void)
{
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextSize(2);
  lcd.setCursor(0, 0);

  if (!readable())
  {
    LOG("ID: read disabled (readable = false)\n");
    lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
    lcd.println("ID: read disabled");
    delay(2000);
    return;
  }

  uint32_t raw = panel()->readCommand(0x0000, 0, 4);
  uint16_t id  = panel()->readRegister(0x0000);
  bool ok = (id == 0x7793);

  LOG("ID: 0x%04X %s (raw bytes: %02X %02X %02X %02X)\n", id,
      ok ? "OK" : "- read failed, shield buffers may be one-way",
      (unsigned)(raw & 0xFF), (unsigned)((raw >> 8) & 0xFF), (unsigned)((raw >> 16) & 0xFF), (unsigned)(raw >> 24));

  lcd.setTextColor(ok ? TFT_GREEN : TFT_RED, TFT_BLACK);
  lcd.printf("ID: 0x%04X %s\n", id, ok ? "OK" : "FAIL");
  if (!ok)
  {
    lcd.setTextSize(1);
    lcd.println("read failed - shield buffers");
    lcd.println("may be one-way (write only)");
  }
  delay(3000);
}

// Register readback after a read: checks that writes still reach the controller.
static void testRegisters(void)
{
  if (!readable()) { return; }

  struct reg_test_t { uint16_t reg; uint16_t value; bool write; };
  static constexpr reg_test_t tests[] =
  {
    { 0x0003, 0x1030, false }, // entry mode set by init (rotation 0, BGR)
    { 0x0400, 0x6200, false }, // NL = 400 lines
    { 0x0404, 0x0055, true  }, // VL (scroll disabled, no visible effect)
  };

  int errors = 0;
  for (auto& t : tests)
  {
    if (t.write)
    {
      lcd.startWrite();
      panel()->writeRegister(t.reg, t.value);
      lcd.endWrite();
    }
    uint16_t got = panel()->readRegister(t.reg);
    if (got != t.value)
    {
      ++errors;
      LOG("  R%03Xh %s 0x%04X read 0x%04X\n", t.reg, t.write ? "wrote" : "expect", t.value, got);
    }
  }
  LOG("Registers: %s\n", errors ? "FAIL" : "OK");
  lcd.startWrite();
  panel()->writeRegister(0x0404, 0x0000);
  lcd.endWrite();

  lcd.setTextSize(2);
  lcd.setTextColor(errors ? TFT_RED : TFT_GREEN, TFT_BLACK);
  lcd.setCursor(0, 40);
  lcd.printf("regs: %s (%d err)\n", errors ? "FAIL" : "OK", errors);
  delay(2000);
}

// Draws known colors, reads them back with readPixel and compares.
static void testReadPixel(void)
{
  if (!readable()) { return; }

  static constexpr uint16_t colors[] = { 0xF800, 0x07E0, 0x001F, 0xFFFF, 0x0000, 0xA5C3, 0x5A3C, 0x8410 };
  static constexpr int n = sizeof(colors) / sizeof(colors[0]);
  static constexpr int size = 20;

  lcd.fillScreen(TFT_BLACK);
  for (int i = 0; i < n; ++i)
  {
    lcd.fillRect(10 + (i % 4) * (size + 10), 10 + (i / 4) * (size + 10), size, size, colors[i]);
  }

  int errors = 0;
  for (int i = 0; i < n; ++i)
  {
    int x = 10 + (i % 4) * (size + 10) + size / 2;
    int y = 10 + (i / 4) * (size + 10) + size / 2;
    uint16_t got = lcd.readPixel(x, y);
    if (got != colors[i])
    {
      ++errors;
      LOG("  (%3d,%3d) wrote 0x%04X read 0x%04X\n", x, y, colors[i], got);
    }
  }

  LOG("readPixel: %s (%d/%d)\n", errors ? "FAIL" : "OK", n - errors, n);
  lcd.setTextSize(2);
  lcd.setCursor(10, 80);
  lcd.setTextColor(errors ? TFT_RED : TFT_GREEN, TFT_BLACK);
  lcd.printf("readPixel %s", errors ? "FAIL" : "OK");
  delay(3000);
}
#endif // ST7793_READ_TEST

static void drawTestScreen(void)
{
  lcd.fillScreen(TFT_BLACK);

  // Color bars
  static constexpr uint16_t colors[] = { TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE, TFT_YELLOW, TFT_CYAN, TFT_MAGENTA };
  int n = sizeof(colors) / sizeof(colors[0]);
  int bar_w = lcd.width() / n;
  for (int i = 0; i < n; ++i)
  {
    lcd.fillRect(i * bar_w, 0, bar_w, lcd.height() / 3, colors[i]);
  }

  // Corner markers to check orientation / mirroring
  lcd.fillRect(0, lcd.height() - 10, 10, 10, TFT_RED);                 // bottom-left  red
  lcd.fillRect(lcd.width() - 10, lcd.height() - 10, 10, 10, TFT_BLUE); // bottom-right blue

  lcd.drawRect(0, 0, lcd.width(), lcd.height(), TFT_WHITE);

  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextSize(2);
  lcd.setCursor(8, lcd.height() / 3 + 10);
  lcd.printf("ST7793 %dx%d\n", lcd.width(), lcd.height());
  lcd.setCursor(8, lcd.getCursorY());
  lcd.printf("rotation %d", lcd.getRotation());
}

// Bouncing ball at maximum frame rate (~20 s), redrawing the FULL screen every frame
// to measure throughput. A full-screen 16-bit sprite (192 KB) does not fit without
// PSRAM, so the frame is rendered in horizontal bands through one band-sized sprite.
static void bouncingBall(uint32_t duration_ms)
{
  static constexpr int r     = 16; // ball radius
  static constexpr int cell  = 20; // checkerboard cell size
  static constexpr int fps_h = 20; // FPS text area at the top

  const int w = lcd.width();
  const int h = lcd.height();

  // Largest band that fits, tallest first.
  LGFX_Sprite spr(&lcd);
  spr.setColorDepth(16);
  static constexpr int band_heights[] = { 50, 40, 25, 20, 10 };
  int band_h = 0;
  for (int bh : band_heights)
  {
    if (spr.createSprite(w, bh)) { band_h = bh; break; }
  }
  if (!band_h)
  {
    LOG("bouncingBall: sprite alloc failed\n");
    return;
  }
  LOG("bouncingBall: %dx%d, band %dx%d (%u B), free heap %u B\n",
      w, h, w, band_h, (unsigned)(w * band_h * 2), (unsigned)FREE_HEAP());

  int x = w / 2, y = h / 2; // ball center
  int vx = 3, vy = 2;
  int scroll = 0;           // checkerboard offset, moves every frame (shows tearing)
  float fps = 0;

  uint32_t start = millis();
  uint32_t fps_time = start;
  uint32_t frames = 0;

  lcd.startWrite();
  while (millis() - start < duration_ms)
  {
    x += vx;
    y += vy;
    if (x - r < 0)     { x = r;         vx = -vx; }
    if (x + r > w - 1) { x = w - 1 - r; vx = -vx; }
    if (y - r < 0)     { y = r;         vy = -vy; }
    if (y + r > h - 1) { y = h - 1 - r; vy = -vy; }
    scroll = (scroll + 1) % (cell * 2);

    for (int by = 0; by < h; by += band_h)
    {
      // Background: checkerboard scrolling to the right.
      spr.fillScreen(TFT_DARKGREY);
      for (int cy = (by / cell) * cell; cy < by + band_h; cy += cell)
      {
        for (int cx = scroll - cell * 2; cx < w; cx += cell)
        {
          if ((((cx - scroll) / cell) + (cy / cell)) & 1)
          {
            spr.fillRect(cx, cy - by, cell, cell, TFT_NAVY);
          }
        }
      }

      // Ball, only if it crosses this band (the sprite clips the rest).
      if (y + r >= by && y - r < by + band_h)
      {
        spr.fillCircle(x, y - by, r, TFT_ORANGE);
        spr.fillCircle(x - r / 3, y - by - r / 3, r / 4, TFT_WHITE);
      }

      // FPS text, drawn every frame on top of everything.
      if (by < fps_h)
      {
        spr.fillRect(0, -by, 150, fps_h, TFT_BLACK);
        spr.setTextSize(2);
        spr.setTextColor(TFT_WHITE);
        spr.setCursor(4, 2 - by);
        spr.printf("FPS %5.1f", fps);
      }

      spr.pushSprite(0, by);
    }
    ++frames;

    uint32_t now = millis();
    if (now - fps_time >= 1000)
    {
      fps = frames * 1000.0f / (now - fps_time);
      frames = 0;
      fps_time = now;
      LOG("FPS: %.1f, %.2f MB/s\n", fps, fps * w * h * 2 / 1000000.0f);
      YIELD_CPU(); // once per second, outside the measured interval
      fps_time = millis();
    }
  }
  lcd.endWrite();

  spr.deleteSprite();
}

void setup()
{
#if defined (ARDUINO)
  Serial.begin(115200);
#endif
  lcd.init();
  LOG("LCD init done\n");

#if ST7793_READ_TEST
  // A failed read does not stop the drawing tests.
  testReadID();
  testRegisters();
  testReadPixel();
#endif
}

void loop()
{
  static constexpr uint16_t fills[] = { TFT_RED, TFT_GREEN, TFT_BLUE };
  for (auto c : fills)
  {
    lcd.fillScreen(c);
    delay(500);
  }

  for (int r = 0; r < 4; ++r)
  {
    lcd.setRotation(r);
    drawTestScreen();
    delay(3000);
  }
  bouncingBall(10000); // in the last rotation
  lcd.setRotation(0);
}

#if !defined (ARDUINO)
extern "C" void app_main(void)
{
  setup();
  for (;;) { loop(); }
}
#endif
