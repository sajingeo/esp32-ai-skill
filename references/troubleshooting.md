# Troubleshooting

Work through these in order. Report what you observed: the exact error line
and the exact port name.

## The port doesn't show up

1. Use a **data** USB-C cable; many cables are charge-only.
2. List ports: `scripts/find-port.sh --all`.
   - macOS: `ls /dev/cu.usbmodem*`
   - Linux: `ls /dev/ttyACM*`, and `dmesg | tail` should show `cdc_acm`
3. The port belongs to the running firmware's USB stack. If the app crashed
   early, or reconfigured GPIO19/20, the port can vanish. Enter **download
   mode** (below) and the ROM bootloader's port will appear.
4. Linux: `Permission denied` means you need `sudo usermod -aG dialout $USER`,
   then a re-login.

## Download mode (when flashing fails)

Symptoms: `Failed to connect to ESP32-S3`, `Waiting for power-on sync`, or
`No serial data received`.

1. Power the board off completely: unplug USB, and if a battery is connected,
   hold PWR for 6 s.
2. Hold **BOOT** (GPIO0), plug USB back in or power on, then release BOOT.
3. Flash: `idf.py -p PORT flash`.
4. **Power-cycle again afterwards.** In download mode the board doesn't start
   the new app by itself.

Normally the onboard auto-download circuit makes this unnecessary. Use it
only when the app has broken USB.

## The flash succeeds but nothing runs

- Run `idf.py -p PORT monitor` and read the boot log.
- A `PSRAM` or `spiram` error means `CONFIG_SPIRAM=y` and
  `CONFIG_SPIRAM_MODE_OCT=y` are missing. Copy `sdkconfig.defaults` from the
  template, delete `sdkconfig`, and run `idf.py set-target esp32s3` again.
- `app partition is too small` means the custom `partitions.csv` isn't
  selected. Check `CONFIG_PARTITION_TABLE_CUSTOM=y`.
- A boot loop with `rst:0xc (RTC_SW_CPU_RST)`: decode the backtrace that the
  monitor prints. A stack overflow in the LVGL task is common; increase its
  stack in menuconfig under LVGL port.

## The screen stays black

1. Run Waveshare's `00_board_check`; it prints display geometry and BSP status.
2. Scan I2C (`08_i2c_tools`):
   - **0x20 missing:** the I/O expander isn't responding, so display power and
     resets never get released.
   - **Neither 0x15 nor 0x38 present:** no touch controller was found.
3. Check `bsp_display_start()` isn't returning `NULL`.
4. Brightness: call `bsp_display_brightness_set(80)` after start. Some
   AMOLED panels come up at 0.
5. V1 versus V2: an old BSP (`^1`) on a V2 board, or hand-written SH8601
   init code on a CO5300 panel, gives a black or garbled screen. Use BSP
   `^2.0.3`.

## The image is shifted, or touch doesn't line up

The CO5300 panel (V2) needs a 0x10 X offset, which BSP 2.0.3 applies when it
detects the V2 touch controller. If you drive the panel yourself, add the
offset. If touch is mirrored, check the rotation passed to
`bsp_display_rotate()`.

## I2C devices are missing

- All onboard devices share GPIO15/14. A second `i2c_new_master_bus()` on the
  same pins breaks the BSP's bus. Use `bsp_i2c_get_handle()`.
- The QMI8658 may answer at 0x6A instead of 0x6B. Probe both.
- Touch only responds after the expander releases touch reset (EXIO2).

## No sound

- Check the speaker is connected. The amplifier is enabled by GPIO46, and
  `bsp_audio_codec_speaker_init()` does that.
- Volume: `esp_codec_dev_set_out_vol(spk, 40)` and up.
- Match the sample rate and bit width you open with to your PCM data.

## The SD card won't mount

- The card must be FAT32 and fully inserted.
- `CONFIG_BSP_SD_FORMAT_ON_MOUNT_FAIL` should stay off; turning it on erases
  the card.
- Check the log for `sdmmc_card_init failed`. Retry with another card.

## Start over cleanly

```sh
rm -rf build sdkconfig managed_components dependencies.lock
idf.py set-target esp32s3 && idf.py build
```

`idf.py -p PORT erase-flash` wipes NVS and Wi-Fi credentials as well. Tell the
human before running it.
