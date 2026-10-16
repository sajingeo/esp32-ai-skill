# Pinout: ESP32-S3-Touch-AMOLED-1.8

Sources:
- the BSP header `include/bsp/esp32_s3_touch_amoled_1_8.h` (component `waveshare/esp32_s3_touch_amoled_1_8` v2.0.3)
- Waveshare's example `pin_config.h` and `board_variant.c` in github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.8
- the Waveshare wiki and the board schematic

Rows marked *inferred* aren't stated in text anywhere. Check them against the
pinout image on the Waveshare docs page or the board's silkscreen before you
wire anything.

## ESP32-S3 GPIO map

| GPIO | Function | BSP macro | Notes |
|---|---|---|---|
| 0 | BOOT button | — | Low when pressed. Hold at power-on for download mode. Usable as an input at runtime |
| 1 | SD card CMD (SPI MOSI) | `BSP_SD_CMD` | microSD, 1-bit SDMMC |
| 2 | SD card CLK (SPI SCK) | `BSP_SD_CLK` | |
| 3 | SD card D0 (SPI MISO) | `BSP_SD_D0` | |
| 4 | Display QSPI D0 | `BSP_LCD_DATA0` | |
| 5 | Display QSPI D1 | `BSP_LCD_DATA1` | |
| 6 | Display QSPI D2 | `BSP_LCD_DATA2` | |
| 7 | Display QSPI D3 | `BSP_LCD_DATA3` | |
| 8 | I2S data out → codec DSDIN (speaker) | `BSP_I2S_DOUT` | |
| 9 | I2S bit clock | `BSP_I2S_SCLK` | |
| 10 | I2S data in ← codec ASDOUT (microphone) | `BSP_I2S_DSIN` | |
| 11 | Display QSPI clock | `BSP_LCD_PCLK` | |
| 12 | Display QSPI chip select | `BSP_LCD_CS` | |
| 14 | I2C SCL | `BSP_I2C_SCL` | Shared board I2C bus, 400 kHz by default |
| 15 | I2C SDA | `BSP_I2C_SDA` | |
| 16 | I2S MCLK | `BSP_I2S_MCLK` | |
| 19 / 20 | USB D− / D+ | — | Native USB-Serial-JTAG, the Type-C port. Don't reuse |
| 21 | Touch interrupt | `BSP_LCD_TOUCH_INT` | |
| 43 / 44 | UART0 TX / RX | — | ESP32-S3 default UART0. Broken out to the UART pad (*inferred* from the schematic net names U0TXD/U0RXD) |
| 45 | I2S word select (LRCK) | `BSP_I2S_LCLK` | Strapping pin |
| 46 | Speaker amplifier enable (NS4150B CTRL) | `BSP_POWER_AMP_IO` | Strapping pin |

There is no backlight or LCD reset GPIO: an AMOLED has no backlight, and its
brightness is set over QSPI. Display and touch resets go through the I/O
expander below.

## Probably free for your own use

The board breaks out **7 GPIOs, plus the I2C, UART and USB pads**, on
1.27 mm-pitch pads. The schematic net list shows these ESP32-S3 GPIOs with no
onboard function:

**GPIO 17, 18, 38, 39, 40, 41, 42** (*inferred*: seven pins, matching the 7
broken-out GPIO pads)

Confirm against the pinout image before wiring. To borrow the board I2C bus
for an external sensor, use GPIO15/14 and check that its address doesn't clash
with the table below.

## I/O expander (TCA9554, I2C 0x20)

| Pin | Function | Notes |
|---|---|---|
| EXIO0 | LCD reset | |
| EXIO1 | Display power enable | |
| EXIO2 | Touch reset | |
| EXIO4 | PWR button | High when pressed. Hold 6 s to power off |
| EXIO7 | SD card CS | Used only in SPI mode; the BSP uses SDMMC |

The BSP initializes the expander (`bsp_io_expander_init()`) and sequences
these pins. If you write your own driver, set the pins as outputs (register
0x03), then drive register 0x01. Waveshare's `board_variant.c` writes the SD
CS bit first, waits 20 ms, then raises LCD reset, display power and touch
reset, and waits 150 ms.

## I2C devices (bus on GPIO15/14)

| Address | Device | Notes |
|---|---|---|
| 0x15 | CST820 touch (V2 boards) | Driven with the `esp_lcd_touch_cst816s` driver |
| 0x18 | ES8311 audio codec, control | Audio itself flows over I2S |
| 0x20 | TCA9554 I/O expander | |
| 0x34 | AXP2101 power management | Battery charging, rails, PWR key |
| 0x38 | FT3168 touch (V1 boards) | |
| 0x51 | PCF85063A RTC | Has a backup-battery pad |
| 0x6B | QMI8658 6-axis IMU | 0x6A on some boards; Waveshare's example probes both |

Only one touch controller is present: 0x15 means a V2 board, 0x38 means V1.
Run the `08_i2c_tools` example or an I2C scan to confirm.

## Memory and display

| Item | Value |
|---|---|
| Flash | 16 MB, QIO (`CONFIG_ESPTOOLPY_FLASHSIZE_16MB`) |
| PSRAM | 8 MB octal, 80 MHz (`CONFIG_SPIRAM_MODE_OCT`) |
| Display | 368 × 448, RGB565, QSPI on SPI2 (`BSP_LCD_H_RES`, `BSP_LCD_V_RES`) |
| Panel driver | V1: SH8601. V2: CO5300 (the BSP applies a 0x10 X offset) |
| Battery | 3.7 V Li-ion on an MX1.25 2-pin connector, charged by the AXP2101 |
