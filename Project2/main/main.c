#include <stdio.h>
#include "esp32s3.h"
#include "app_ui.h"
#include "nvs_flash.h"
#include <esp_system.h>

static const char *TAG = "main";

EventGroupHandle_t my_event_group;

static void page_task(void *pvParameters)
{
    Main_Page();
    vTaskDelete(NULL);
}

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

    lv_gui_start(); // 显示开机界面
    vTaskDelay(200);

    my_event_group = xEventGroupCreate();
    
    xTaskCreatePinnedToCore(page_task, "page_task", 4 * 1024, NULL, 5, NULL, 0);
    

}