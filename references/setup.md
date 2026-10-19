# Getting started: toolchain, build, flash

The board is programmed with **ESP-IDF v5.5.x or v6.0.x**. Waveshare's CI builds
its examples with v5.5.5 and v6.0.2. Arduino also works (see the end), but this
skill defaults to ESP-IDF because that's what the managed BSP targets.

## 1. Install ESP-IDF with EIM (once per machine)

Use EIM, Espressif's ESP-IDF Installation Manager. It installs ESP-IDF and its
tools, and can keep several IDF versions side by side.

**macOS (Homebrew), the recommended path**

```sh
brew tap espressif/eim
brew install eim                 # CLI. For the GUI instead: brew install --cask eim-gui
eim install                      # installs ESP-IDF (non-interactive by default)
```

`eim install` takes options:
- `-i v5.5.5` pins a version. Use v5.5.x or v6.0.x for this board.
- `-t esp32s3` installs only the ESP32-S3 toolchain.
- `-p <dir>` sets the install location.

`eim wizard` walks through the same choices interactively.

**Activate ESP-IDF in a shell** before running `idf.py`, using any of these:

```sh
eim shell                        # new shell with the selected IDF active (exit to leave)
eim run "idf.py build"           # run a single command with IDF active
source ~/.espressif/tools/activate_idf_v5.5.5.sh   # or source the activation script
```

The script name depends on the version you installed, and `eim select` lists
installed versions with their script paths. A handy alias:
`alias get_idf='source ~/.espressif/tools/activate_idf_v5.5.5.sh'`.

Other EIM commands:
- `eim list` shows installed versions.
- `eim select` sets the active one.
- `eim fix` repairs a broken install.

**Linux:** EIM installs the same way through Homebrew, or through Espressif's
APT/DNF/pacman repositories (`eim-cli` package). See the EIM docs in `links.md`.
Then run `eim install` as above.

**Windows:** `winget install Espressif.EIM-CLI` (or `Espressif.EIM` for the
GUI), then `eim install`.

**Check the install:** `idf.py --version` in an activated shell should report
the version you installed.

<details>
<summary>Manual install without EIM</summary>

```sh
mkdir -p ~/esp && cd ~/esp
git clone -b v5.5.5 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && ./install.sh esp32s3 && . ./export.sh
```

</details>

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
- `timeout 8 idf.py -p PORT monitor`: grab the boot log non-interactively (no
  TTY needed) — useful for agent-driven boot checks. In non-interactive mode
  the monitor reads commands from stdin: `reset`, `send <text>`, `sleep <s>`,
  `expect [--timeout <s>] <regex>`, `exit`.
- `idf.py -p PORT erase-flash`: wipe everything, including NVS and Wi-Fi
  credentials. Say so before running it.

ESP-IDF **v6.0.2 is confirmed** building this skill's template (resolves LVGL
9.6.0, BSP 2.0.3). v6.1 is outside the template's `idf` range and untested.

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
