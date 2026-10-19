---
name: esp32-ai-skill
description: Build, flash and debug firmware for the Waveshare ESP32-S3-Touch-AMOLED-1.8 board (ESP32-S3R8, 1.8" 368x448 AMOLED with touch, IMU, RTC, ES8311 audio, microSD, AXP2101 PMU) using ESP-IDF and Waveshare's managed BSP. Use when creating a project for this board, wiring or choosing GPIOs, flashing or recovering it, or debugging its display, touch, sensors, audio or SD card.
---

# Waveshare ESP32-S3-Touch-AMOLED-1.8

This skill helps you write and flash firmware for one specific board. Read the
reference that matches the task:

| Task | Read |
|---|---|
| Which GPIO does what, which pins are free, I2C addresses | `references/pinout.md` |
| Install ESP-IDF, create a project, build, flash, monitor | `references/setup.md` |
| Display, touch, LVGL, brightness, SD, audio, IMU, RTC code | `references/bsp-api.md` |
| Board won't flash, no port, blank screen, I2C devices missing | `references/troubleshooting.md` |
| Waveshare and Espressif documentation links | `references/links.md` |

## Facts to keep in mind

- **SoC:** ESP32-S3R8, dual-core LX7 at 240 MHz, **16 MB flash, 8 MB octal
  PSRAM**. Build with target `esp32s3`.
- **USB:** the Type-C port is the ESP32-S3's native USB (USB-Serial-JTAG). It
  shows up as `/dev/cu.usbmodem*` on macOS, `/dev/ttyACM*` on Linux, and a COM
  port on Windows. There is no separate UART bridge chip.
- **Two hardware revisions:**
  - V1: SH8601 panel and FT3168 touch (I2C 0x38)
  - V2: CO5300 panel and CST820 touch (I2C 0x15). V2 ships from May 2026.

  The revision is printed on the back of the board. BSP `waveshare/esp32_s3_touch_amoled_1_8` `^2.0.3` detects which touch chip is present and handles both.
- **Display and touch reset are not on GPIOs.** They sit on a TCA9554 I/O
  expander (I2C 0x20): EXIO0 = LCD reset, EXIO1 = display power, EXIO2 =
  touch reset, EXIO4 = PWR button, EXIO7 = SD card CS. The BSP drives these;
  only touch them yourself in bare-metal code.
- **Shared I2C bus:** SDA = GPIO15, SCL = GPIO14 carries touch, the I/O
  expander, AXP2101 (0x34), PCF85063 RTC (0x51), QMI8658 IMU (0x6B, or 0x6A),
  and the ES8311 codec's control port (0x18).
- **Don't reuse board pins.** GPIO 1–12, 14–16, 21, 45 and 46 are taken by
  the SD card, display, I2C, touch interrupt and audio. GPIO 19/20 are USB.
  Use `references/pinout.md` before assigning any GPIO.

## Default workflow

1. **Confirm the toolchain:** `idf.py --version` should report ESP-IDF v5.5.x
   or v6.0.x. If `idf.py` isn't found, ESP-IDF is probably installed with EIM
   but not activated. Run builds through `eim run "idf.py build"`, or ask the
   human to open `eim shell`. If ESP-IDF isn't installed, the setup is
   `brew tap espressif/eim && brew install eim && eim install` (see
   `references/setup.md`).
2. **Start from the template:**
   `scripts/new-project.sh <name> [dest-dir]` copies `templates/hello_amoled`,
   which already has the BSP dependency, PSRAM, 16 MB flash and a partition
   table sized for LVGL.
3. **Build, flash and watch:**
   ```sh
   idf.py set-target esp32s3      # once per project
   idf.py build
   idf.py -p "$(scripts/find-port.sh)" flash monitor
   ```
   Quit the monitor with `Ctrl-]`.
4. **Read the boot log.** The template prints the detected board facts and
   the touch count. Report what the log shows, not what you expected.
5. **If flashing fails or the port disappears:** use download mode. Hold BOOT,
   power-cycle the board, release BOOT, flash, then power-cycle again; the
   board stays in download mode until you do. Details are in
   `references/troubleshooting.md`.

## Rules

- **Use the BSP** for display, touch, SD and audio (`#include "bsp/esp-bsp.h"`)
  instead of hand-rolling QSPI panel init. It handles both revisions and the
  I/O expander sequencing.
- **Lock LVGL.** Wrap every LVGL call outside LVGL's own task in
  `bsp_display_lock()` / `bsp_display_unlock()`.
- **Keep PSRAM enabled** (`CONFIG_SPIRAM=y`, octal mode). Display and LVGL
  examples fail without it.
- **Audio:** start at low volume. The speaker amplifier is enabled by GPIO46,
  and `bsp_audio_codec_speaker_init()` handles it.
- **Ask before irreversible actions:** never burn eFuses, enable flash
  encryption or enable secure boot without asking the human. `idf.py erase-flash`
  is safe but wipes NVS and Wi-Fi credentials, so say so before running it.
