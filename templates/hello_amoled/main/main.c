/*
 * hello_amoled: starting point for the Waveshare ESP32-S3-Touch-AMOLED-1.8.
 *
 * Brings up the AMOLED panel, touch and LVGL through Waveshare's BSP, shows a
 * title and a tap counter, and logs board facts plus every tap over USB serial
 * so a person or an agent can confirm the firmware is alive from the monitor.
 */

#include <inttypes.h>
#include <stdio.h>

#include "bsp/esp-bsp.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

static const char *TAG = "hello_amoled";

static lv_obj_t *s_count_label;
static uint32_t s_taps;

#if LVGL_VERSION_MAJOR >= 9
#define ACTIVE_SCREEN() lv_screen_active()
#define BUTTON_CREATE(parent) lv_button_create(parent)
#else
#define ACTIVE_SCREEN() lv_scr_act()
#define BUTTON_CREATE(parent) lv_btn_create(parent)
#endif

static void log_board_facts(void)
{
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);

    ESP_LOGI(TAG, "chip: ESP32-S3 rev %d, %d cores", chip.revision, chip.cores);
    ESP_LOGI(TAG, "flash: %" PRIu32 " MB, PSRAM free: %u KB",
             flash_size / (1024 * 1024),
             (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
    ESP_LOGI(TAG, "display: %dx%d, I2C SDA=%d SCL=%d, touch INT=%d",
             BSP_LCD_H_RES, BSP_LCD_V_RES, BSP_I2C_SDA, BSP_I2C_SCL, BSP_LCD_TOUCH_INT);
}

/* Runs inside LVGL's task, so no display lock is needed here. */
static void on_tap(lv_event_t *e)
{
    (void)e;
    s_taps++;
    lv_label_set_text_fmt(s_count_label, "%" PRIu32, s_taps);
    ESP_LOGI(TAG, "tap %" PRIu32, s_taps);
}

static void build_ui(void)
{
    lv_obj_t *screen = ACTIVE_SCREEN();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0B0F14), LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Hello, AMOLED");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF2A541), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 48);

    s_count_label = lv_label_create(screen);
    lv_label_set_text(s_count_label, "0");
    lv_obj_set_style_text_color(s_count_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_count_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(s_count_label, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t *button = BUTTON_CREATE(screen);
    lv_obj_set_size(button, 200, 64);
    lv_obj_align(button, LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x5CC8D6), LV_PART_MAIN);
    lv_obj_add_event_cb(button, on_tap, LV_EVENT_CLICKED, NULL);

    lv_obj_t *button_label = lv_label_create(button);
    lv_label_set_text(button_label, "Tap me");
    lv_obj_set_style_text_color(button_label, lv_color_hex(0x0B0F14), LV_PART_MAIN);
    lv_obj_center(button_label);
}

void app_main(void)
{
    log_board_facts();

    lv_display_t *display = bsp_display_start();
    if (display == NULL) {
        ESP_LOGE(TAG, "display init failed: check I2C (expander 0x20, touch 0x15/0x38) in the log above");
        abort();
    }
    ESP_ERROR_CHECK(bsp_display_brightness_set(80));

    if (bsp_display_lock(1000)) {
        build_ui();
        bsp_display_unlock();
    } else {
        ESP_LOGE(TAG, "could not lock LVGL to build the UI");
    }

    ESP_LOGI(TAG, "ready: tap the button on screen");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
