#include <stdio.h>
#include "esp32s3.h"
#include "app_ui.h"
#include "nvs_flash.h"
#include <esp_system.h>

static const char *TAG = "main";

void app_main(void)
{
    //  Initialize NVS.
     esp_err_t ret = nvs_flash_init();
     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
         ESP_ERROR_CHECK(nvs_flash_erase());
         ret = nvs_flash_init();
     }
     ESP_ERROR_CHECK( ret );

    i2c_init();
    pca9557_init();
    lvgl_start();
    qmi8658_init();
    bsp_spiffs_mount();
    bsp_codec_init(); // 音频初始化

    
    Main_Page();
}