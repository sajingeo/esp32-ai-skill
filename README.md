# esp32-ai-skill

A Claude skill for the **Waveshare ESP32-S3-Touch-AMOLED-1.8**: an ESP32-S3
dev board with a 1.8" 368×448 AMOLED touchscreen, 6-axis IMU, RTC, ES8311
audio, microSD and battery charging.

With the skill installed, Claude knows the board's pinout and its two hardware
revisions, how to set up ESP-IDF, build, flash and recover the board, and how
to drive the display, touch, SD, audio, IMU and RTC through Waveshare's
board support package. It starts new projects from a ready-to-build template.

## Install

**Claude Code** (personal skill):

```sh
git clone https://github.com/sajingeo/esp32-ai-skill.git ~/.claude/skills/esp32-ai-skill
```

Or put it in one project only: clone into `<project>/.claude/skills/esp32-ai-skill`.

**Claude apps (claude.ai / desktop):** download this repo as a ZIP and upload
it as a custom skill. See Anthropic's help center for where skills live in
your plan's settings.

Then just ask, for example:
- "Make a new project for my AMOLED board that shows the IMU tilt as a bubble level."
- "Which GPIOs can I use for an external sensor?"
- "Flashing fails with *Failed to connect*. Help."

## Get started by hand

```sh
# 1. ESP-IDF via EIM, the ESP-IDF Installation Manager (details: references/setup.md)
brew tap espressif/eim && brew install eim
eim install                      # pin a version with: eim install -i v5.5.5
eim shell                        # opens a shell with ESP-IDF active

# 2. New project from the template
./scripts/new-project.sh my_app ~/projects && cd ~/projects/my_app

# 3. Build and flash over the Type-C port
idf.py set-target esp32s3 && idf.py build
idf.py -p "$(~/.claude/skills/esp32-ai-skill/scripts/find-port.sh)" flash monitor
```

The template shows "Hello, AMOLED" with a tap counter and logs board facts
and every tap to the serial monitor.

## Board at a glance

| | |
|---|---|
| SoC | ESP32-S3R8 · 16 MB flash · 8 MB octal PSRAM |
| Display | 1.8" AMOLED 368×448 over QSPI (V1 SH8601, V2 CO5300) |
| Touch | V1 FT3168 (0x38) · V2 CST820 (0x15) |
| I2C bus | SDA 15 / SCL 14: touch, TCA9554 expander 0x20, AXP2101 0x34, PCF85063 0x51, QMI8658 0x6B, ES8311 0x18 |
| Audio | ES8311 over I2S (MCLK 16, BCLK 9, WS 45, DOUT 8, DIN 10), speaker amp enable 46 |
| microSD | SDMMC 1-bit: CMD 1, CLK 2, D0 3 |
| USB | Native USB-Serial-JTAG on the Type-C port |
| Buttons | BOOT = GPIO0 · PWR = expander EXIO4 |

The full map, including which pins are free, is in [references/pinout.md](references/pinout.md).

## Contents

```
SKILL.md                       what Claude reads first
references/pinout.md           GPIOs, I/O expander, I2C addresses, free pins
references/setup.md            ESP-IDF install, build, flash, examples, Arduino
references/bsp-api.md          display/touch/LVGL, SD, audio, IMU, RTC code
references/troubleshooting.md  ports, download mode, black screen, I2C
references/links.md            Waveshare and Espressif documentation
templates/hello_amoled/        ESP-IDF project template
scripts/new-project.sh         copy the template
scripts/find-port.sh           find the board's serial port
```

## Sources

Pin assignments come from Waveshare's BSP component
(`waveshare/esp32_s3_touch_amoled_1_8` v2.0.3), their example code at
[waveshareteam/ESP32-S3-Touch-AMOLED-1.8](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.8),
the [Waveshare docs](https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.8)
and the board schematic. Anything inferred rather than stated is marked so in
`pinout.md`.
