# Programming the board with the BSP

Component: `waveshare/esp32_s3_touch_amoled_1_8` (v2.0.3), from the ESP
Component Registry. Include it with `#include "bsp/esp-bsp.h"`. It pulls in
LVGL (v8 or v9), `esp_lvgl_port`, the CO5300/SH8601 panel drivers, the
CST816S/FT5x06 touch drivers, `esp_codec_dev` and the TCA9554 expander driver.

The snippets below use only functions declared in the BSP's public headers.
For complete programs, see Waveshare's examples (paths are given in each
section).

## Display and touch (LVGL)

```c
#include "bsp/esp-bsp.h"
#include "lvgl.h"

void app_main(void)
{
    lv_display_t *disp = bsp_display_start();   // panel + touch + LVGL task
    if (disp == NULL) {
        abort();
    }
    bsp_display_brightness_set(80);             // 0-100 %

    bsp_display_lock(0);                        // required around LVGL calls
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello AMOLED");
    lv_obj_center(label);
    bsp_display_unlock();
}
```

- `bsp_display_start()` returns `NULL` on failure. Check the log for I2C or
  I/O expander errors.
- `bsp_display_lock(timeout_ms)` returns `false` on timeout; 0 means wait
  forever. Every LVGL call outside LVGL's own callbacks must be inside a lock.
  **For a UI update that must land, lock with 0.** A short timeout silently
  drops the update when a heavy frame (e.g. a large scaled GIF) holds the lock
  longer than the timeout — the screen then sticks on the old content while the
  rest of your handler (state vars, sounds) runs on as if it succeeded.
- `bsp_display_get_input_dev()` returns the touch `lv_indev_t`.
- `bsp_display_rotate(disp, LV_DISPLAY_ROTATION_90)` rotates the display.
  That's the LVGL v9 constant; v8 uses `LV_DISP_ROT_90`. The BSP accepts
  either LVGL major version, and the component manager picks v9 by default.
- **You get LVGL v9** (9.6.0 with ESP-IDF v6.0.2). Beyond the v8/v9 names
  above, guard these with `#if LVGL_VERSION_MAJOR >= 9`:
  `lv_anim_set_duration` / `lv_anim_set_playback_duration` / `lv_anim_delete`
  (v8: `lv_anim_set_time` / `..._playback_time` / `lv_anim_del`), and
  `lv_obj_remove_flag` (v8: `lv_obj_clear_flag`). In v9
  `lv_obj_add_flag`/`lv_obj_remove_flag` still compile but warn as deprecated
  (it points you at `lv_obj_set_hidden()` etc.) — harmless.
- Resolution: `BSP_LCD_H_RES` × `BSP_LCD_V_RES` = 368 × 448, RGB565.
- For custom buffer settings, use `bsp_display_start_with_config()`.
- Tearing and refresh mode are set in menuconfig under
  "Board Support Package": full refresh or direct mode.
- Complete examples: `00_bsp_quickstart` (dashboard), `14_lvgl_demo_v9`.

**Without LVGL:** `bsp_display_new()` returns the raw `esp_lcd_panel_handle_t`
for `esp_lcd_panel_draw_bitmap()`, and `bsp_touch_new()` returns an
`esp_lcd_touch_handle_t`. The bare-panel example is `13_display_colorbar`.

## Animated GIFs and large images

Enable the decoder (`CONFIG_LV_USE_GIF=y`) and embed each GIF as an
`lv_image_dsc_t` whose data is the raw file bytes — LVGL reads the real size
from the data, so leave `w`/`h` at 0:

```c
const lv_image_dsc_t my_gif = {
    .header = { .cf = LV_COLOR_FORMAT_RAW, .w = 0, .h = 0 },
    .data_size = sizeof(my_gif_map),   // raw .gif bytes in a uint8_t[]
    .data = my_gif_map,
};

lv_obj_t *g = lv_gif_create(lv_screen_active());
lv_gif_set_src(g, &my_gif);            // plays and loops automatically
lv_image_set_scale(g, 640);            // lv_gif is an lv_image: 256 = 100%
lv_image_set_pivot(g, w/2, h/2);       // so scaling stays centered under lv_obj_align
```

- **Critical gotcha — LVGL's default memory pool is too small for images.**
  On this BSP the component manager defaults LVGL to its builtin allocator
  (`CONFIG_LV_USE_BUILTIN_MALLOC=y`, `LV_MEM_SIZE` 64 KB). A 128×128 ARGB8888
  GIF frame alone needs ~65 KB, so the decode fails and the widget shows
  **nothing** — and since `CONFIG_LV_USE_LOG` is off by default, there is no
  error. Fix: `CONFIG_LV_USE_CLIB_MALLOC=y`, which routes `lv_malloc` to the
  C-library heap (PSRAM-backed, since `CONFIG_SPIRAM_USE_MALLOC=y`). Turn on
  `CONFIG_LV_USE_LOG=y` while debugging a blank image.
- GIF config changes live in `sdkconfig.defaults`; delete `sdkconfig` and
  rebuild so they actually apply (an existing `sdkconfig` is not re-derived).
- The LVGL example is `examples/libs/gif`; the ESP image-converter emits the
  same descriptor shape.

## Reading host input over USB-Serial-JTAG

The Type-C port is the ESP32-S3's native USB. To read bytes the host sends to
the board (e.g. a desktop script pushing status to display), install the driver
and read. It coexists with console logging (`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`)
because RX and TX are independent directions — logs still go out while you read.

```c
#include "driver/usb_serial_jtag.h"   // add esp_driver_usb_serial_jtag to REQUIRES

usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
esp_err_t err = usb_serial_jtag_driver_install(&cfg);
// ESP_ERR_INVALID_STATE means the console already installed it — treat as OK
uint8_t buf[64];
int n = usb_serial_jtag_read_bytes(buf, sizeof(buf), pdMS_TO_TICKS(200));
// assemble lines yourself; read returns whatever arrived within the timeout
```

- From a **Mac host, write to `/dev/cu.usbmodem*`, not `/dev/tty.*`**: the
  call-up (`cu`) device doesn't assert DTR on open, so writing won't reset the
  board. `printf 'line\n' > /dev/cu.usbmodem*` is enough; no baud/stty needed
  (USB CDC ignores baud).
- One owner per direction. `idf.py monitor` (reading TX) and a host writer (to
  RX) can run at the same time, but a host reader and the monitor cannot both
  own the port — quit the monitor before running a host script that reads.

## I2C bus (shared)

```c
ESP_ERROR_CHECK(bsp_i2c_init());                 // GPIO15 SDA / GPIO14 SCL, 400 kHz
i2c_master_bus_handle_t bus = bsp_i2c_get_handle();
// add your own devices with i2c_master_bus_add_device(bus, ...)
```

Always reuse the BSP's bus handle. Don't call `i2c_new_master_bus()` on
GPIO14/15 a second time. `bsp_display_start()` calls `bsp_i2c_init()` itself,
so call `bsp_i2c_get_handle()` after it.

## IMU (QMI8658) and RTC (PCF85063A)

The BSP doesn't drive these (`BSP_CAPS_IMU 0`). Add Waveshare's components:

```yaml
# main/idf_component.yml
dependencies:
  waveshare/qmi8658: "^2.0.0"
  waveshare/pcf85063a: "^2.0.0"
```

```c
#include "qmi8658.h"

qmi8658_dev_t imu = {0};
ESP_ERROR_CHECK(qmi8658_init(&imu, bsp_i2c_get_handle(), QMI8658_ADDRESS_HIGH)); // 0x6B; try QMI8658_ADDRESS_LOW (0x6A) if this fails
qmi8658_set_accel_range(&imu, QMI8658_ACCEL_RANGE_4G);
qmi8658_set_accel_odr(&imu, QMI8658_ACCEL_ODR_250HZ);
qmi8658_set_gyro_range(&imu, QMI8658_GYRO_RANGE_256DPS);
qmi8658_set_gyro_odr(&imu, QMI8658_GYRO_ODR_250HZ);
qmi8658_enable_sensors(&imu, QMI8658_ENABLE_ACCEL | QMI8658_ENABLE_GYRO);

qmi8658_data_t d = {0};
bool ready = false;
if (qmi8658_is_data_ready(&imu, &ready) == ESP_OK && ready &&
    qmi8658_read_sensor_data(&imu, &d) == ESP_OK) {
    printf("accel %.2f %.2f %.2f\n", d.accelX, d.accelY, d.accelZ);
}
```

The RTC follows the same pattern: `pcf85063a_init(&rtc, bus, PCF85063A_ADDRESS)`,
then `pcf85063a_get_time_date()` and `pcf85063a_set_time_date()`. The complete
examples are `92_qmi8658_imu` and `91_pcf85063_rtc`. Waveshare's examples
include a soft-reset and address-probe sequence for the IMU; copy that if
`qmi8658_init()` fails on your board.

## SD card

```c
if (bsp_sdcard_mount() == ESP_OK) {               // mounts at BSP_SD_MOUNT_POINT ("/sdcard")
    FILE *f = fopen(BSP_SD_MOUNT_POINT "/log.txt", "a");
    if (f) {
        fprintf(f, "hello\n");
        fclose(f);
    }
    bsp_sdcard_unmount();
}
```

The SD card uses 1-bit SDMMC (CMD 1, CLK 2, D0 3). Use FAT32-formatted cards.
Keep file names short unless `CONFIG_FATFS_LFN_*` is enabled. The example is
`09_sdmmc`.

## Audio (ES8311 + NS4150B speaker amp)

```c
#include "esp_codec_dev.h"

esp_codec_dev_handle_t spk = bsp_audio_codec_speaker_init();  // also enables the amp on GPIO46
esp_codec_dev_sample_info_t fs = {
    .sample_rate = 16000, .channel = 1, .bits_per_sample = 16,
};
esp_codec_dev_open(spk, &fs);
esp_codec_dev_set_out_vol(spk, 40);           // start quiet
esp_codec_dev_write(spk, pcm, pcm_bytes);     // 16-bit PCM

esp_codec_dev_handle_t mic = bsp_audio_codec_microphone_init();
// esp_codec_dev_open(mic, &fs); esp_codec_dev_read(mic, buf, len);
```

`12_i2s_codec` sets up the I2S channel and codec by hand, if you need full
control.

## SPIFFS

`bsp_spiffs_mount()` mounts the `storage` partition at `/spiffs`. The template's
`partitions.csv` includes that partition.

## Power (AXP2101)

The BSP doesn't wrap the PMU. Use XPowersLib, as in the `90_axp2101_pmu`
example, for battery voltage, charge state and the PWR key. Be careful writing
PMU rail settings: a wrong voltage on a rail can brown out the board. Ask the
human before changing rails.

## Kconfig worth knowing (`idf.py menuconfig` → Board Support Package)

| Option | Default |
|---|---|
| `CONFIG_BSP_I2C_NUM` | 1 |
| `CONFIG_BSP_I2C_CLK_SPEED_HZ` | 400000 |
| `CONFIG_BSP_SD_MOUNT_POINT` | `/sdcard` |
| `CONFIG_BSP_SD_FORMAT_ON_MOUNT_FAIL` | n (keep it off; it erases cards) |
| `CONFIG_BSP_SPIFFS_MOUNT_POINT` | `/spiffs` |
| `CONFIG_BSP_LCD_RGB_BOUNCE_BUFFER_HEIGHT` | 20 |
| `CONFIG_BSP_DISPLAY_LVGL_AVOID_TEAR` | n |
