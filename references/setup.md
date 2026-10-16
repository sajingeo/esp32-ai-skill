# Getting started: toolchain, build, flash

The board is programmed with **ESP-IDF v5.5.x or v6.0.x**. Waveshare's CI builds
its examples with v5.5.5 and v6.0.2. Arduino also works (see the end), but this
skill defaults to ESP-IDF because that's what the managed BSP targets.

## 1. Install ESP-IDF (once per machine)

**macOS / Linux**

```sh
# prerequisites
#   macOS:  brew install cmake ninja dfu-util python3
#   Ubuntu: sudo apt install git wget flex bison gperf python3 python3-venv \
#             cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
mkdir -p ~/esp && cd ~/esp
git clone -b v5.5.5 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && ./install.sh esp32s3
. ./export.sh          # run in every new shell, or add an alias:
# echo "alias get_idf='. $HOME/esp/esp-idf/export.sh'" >> ~/.zshrc
idf.py --version
```

**Windows:** use Espressif's ESP-IDF Tools Installer (link in `links.md`), pick
v5.5.x, then work from the "ESP-IDF PowerShell" shortcut.

**Linux only:** add yourself to the `dialout` group so you can open the port:
`sudo usermod -aG dialout $USER`, then log out and back in.

## 2. Create a project

From this skill's folder:

```sh
scripts/new-project.sh my_app ~/projects
cd ~/projects/my_app
```

That copies `templates/hello_amoled`. To add the board to an existing project
instead, put this in `main/idf_component.yml`:

```yaml
dependencies:
  idf: ">=5.5,<6.1"
  waveshare/esp32_s3_touch_amoled_1_8:
    version: "^2.0.3"
    public: true
```

Add `REQUIRES esp32_s3_touch_amoled_1_8` to `main/CMakeLists.txt`, and copy
the template's `sdkconfig.defaults` and `partitions.csv`. The first build
downloads the BSP and its dependencies into `managed_components/`, so it needs
internet access.

Don't commit `managed_components/`, `dependencies.lock` or `sdkconfig` unless
you mean to pin that state. `sdkconfig.defaults` is the file to version.

## 3. Build, flash, monitor

```sh
idf.py set-target esp32s3            # once; regenerates sdkconfig from sdkconfig.defaults
idf.py build
idf.py -p PORT flash monitor         # PORT from scripts/find-port.sh
```

| OS | Typical port |
|---|---|
| macOS | `/dev/cu.usbmodem1101` (name varies) |
| Linux | `/dev/ttyACM0` |
| Windows | `COM5` (check Device Manager) |

The onboard auto-download circuit normally enters bootloader mode by itself.
Exit the monitor with `Ctrl-]`. To change settings, run `idf.py menuconfig`,
but put anything that should persist into `sdkconfig.defaults`.

Useful extras:
- `idf.py size`: flash and RAM use
- `idf.py -p PORT monitor`: attach to a running board without reflashing
- `idf.py -p PORT erase-flash`: wipe everything, including NVS and Wi-Fi
  credentials. Say so before running it.

## 4. Start from Waveshare's examples

```sh
git clone --depth 1 https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.8.git
cd ESP32-S3-Touch-AMOLED-1.8/examples/esp-idf/00_board_check
idf.py set-target esp32s3 && idf.py build && idf.py -p PORT flash monitor
```

Recommended order:

| Example | What it checks |
|---|---|
| `00_board_check` | Chip, flash, PSRAM and BSP over serial only. Always run this first |
| `00_bsp_quickstart` | Display, touch, brightness and SD in an LVGL dashboard |
| `08_i2c_tools` | Scans the I2C bus when a device seems missing |
| `09_sdmmc` | SD card |
| `10_wifi_station` | Wi-Fi |
| `12_i2s_codec` | Speaker |
| `14_lvgl_demo_v9` | The full LVGL widgets demo |
| `90_axp2101_pmu`, `91_pcf85063_rtc`, `92_qmi8658_imu` | PMU, RTC and IMU diagnostics |

## 5. Restore the factory firmware

Waveshare's repo keeps complete factory images in `firmware/`, written at offset 0x0:

| Board revision | Image |
|---|---|
| V1 (SH8601 / FT3168) | `ESP32-S3-Touch-AMOLED-1.8-FactoryOnly-260915.bin` |
| V2 (CO5300 / CST820) | `ESP32-S3-Touch-AMOLED-1.8-V2-FactoryOnly-260915.bin` |

```sh
esptool.py --chip esp32s3 -p PORT -b 921600 write_flash 0x0 firmware/<image>.bin
```

The file names carry a date stamp, so check the folder for the latest ones. Waveshare's Windows instructions (Flash Download Tool) are linked in
`links.md`.

## Arduino (optional)

Waveshare's CI builds its sketches with Arduino-ESP32 core 3.3.11 and this
FQBN:

```
esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB
```

In Arduino IDE 2, choose **ESP32S3 Dev Module** with Flash Size 16 MB and the
"16M Flash (3MB APP/9.9MB FATFS)" partition scheme. Also set:

- **PSRAM: OPI PSRAM**, for LVGL sketches
- **USB CDC On Boot: Enabled**, so `Serial` prints over the Type-C port

Waveshare's wiki recommends USB CDC On Boot. The OPI PSRAM setting follows
from the board's octal PSRAM; CI doesn't set it because it only compiles.

Waveshare's sketches need the libraries bundled in their repo
(`examples/arduino/libraries` for V1 boards, `examples/arduino-v2/libraries`
for V2). Copy them into your Arduino `libraries` folder.
