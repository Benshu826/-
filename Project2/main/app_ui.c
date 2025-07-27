#include "app_ui.h"
#include "audio_player.h"
#include "esp32s3.h"
#include "file_iterator.h"
#include "string.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "protocol_examples_utils.h"
#include "cJSON.h"
#include "zlib.h"

#include <dirent.h>
#include <time.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <sys/param.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "esp_netif.h"
#include "esp_event.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"


static const char *TAG = "app_ui";
LV_FONT_DECLARE(font_alipuhui20);
LV_FONT_DECLARE(font_led);
LV_FONT_DECLARE(font_myawesome);
LV_FONT_DECLARE(font_qweather);

lv_obj_t * main_obj; // 主界面
lv_obj_t * main_text_label; // 主界面 欢迎语
lv_obj_t * icon_in_obj; // 应用界面
int icon_flag; // 标记现在进入哪个应用 在主界面时为0

/********************************************************* 第1个图标 音乐播放器 应用程序*****************************************************************************/
static audio_player_config_t player_config = {0};
static uint8_t g_sys_volume = VOLUME_DEFAULT;
static file_iterator_instance_t *file_iterator = NULL;

lv_obj_t *music_list;
lv_obj_t *label_play_pause;
lv_obj_t *btn_play_pause;
lv_obj_t *volume_slider;

lv_obj_t *music_title_label;
lv_obj_t *btn_music_back;

// 播放指定序号的音乐
static void play_index(int index)
{
    ESP_LOGI(TAG, "play_index(%d)", index);

    char filename[128];
    int retval = file_iterator_get_full_path_from_index(file_iterator, index, filename, sizeof(filename));
    if (retval == 0) {
        ESP_LOGE(TAG, "unable to retrieve filename");
        return;
    }

    FILE *fp = fopen(filename, "rb");
    if (fp) {
        ESP_LOGI(TAG, "Playing '%s'", filename);
        audio_player_play(fp);
    } else {
        ESP_LOGE(TAG, "unable to open index %d, filename '%s'", index, filename);
    }
}

// 设置声音处理函数
static esp_err_t _audio_player_mute_fn(AUDIO_PLAYER_MUTE_SETTING setting)
{
    esp_err_t ret = ESP_OK;
    // 判断是否需要静音
    bsp_codec_mute_set(setting == AUDIO_PLAYER_MUTE ? true : false);
    // 如果不是静音 设置音量
    if (setting == AUDIO_PLAYER_UNMUTE) {
        bsp_codec_volume_set(g_sys_volume, NULL);
    }
    ret = ESP_OK;

    return ret;
}

// 播放音乐函数 播放音乐的时候 会不断进入
static esp_err_t _audio_player_write_fn(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;

    ret = bsp_i2s_write(audio_buffer, len, bytes_written, timeout_ms);

    return ret;
}

// 设置采样率 播放的时候进入一次
static esp_err_t _audio_player_std_clock(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    esp_err_t ret = ESP_OK;

    // ret = bsp_codec_set_fs(rate, bits_cfg, ch); // 如果播放的音乐固定是16000采样率 这里可以不用打开 如果采样率未知 把这里打开
    return ret;
}

// 回调函数 播放器每次动作都会进入
static void _audio_player_callback(audio_player_cb_ctx_t *ctx)
{
    ESP_LOGI(TAG, "ctx->audio_event = %d", ctx->audio_event);
    switch (ctx->audio_event) {
    case AUDIO_PLAYER_CALLBACK_EVENT_IDLE: {  // 播放完一首歌 进入这个case
        ESP_LOGI(TAG, "AUDIO_PLAYER_REQUEST_IDLE");
        // 指向下一首歌
        file_iterator_next(file_iterator);
        int index = file_iterator_get_index(file_iterator);
        ESP_LOGI(TAG, "playing index '%d'", index);
        play_index(index);
        // 修改当前播放的音乐名称
        lvgl_port_lock(0);
        lv_dropdown_set_selected(music_list, index);
        lvgl_port_unlock();
        break;
    }
    case AUDIO_PLAYER_CALLBACK_EVENT_PLAYING: // 正在播放音乐
        ESP_LOGI(TAG, "AUDIO_PLAYER_REQUEST_PLAY");
        pa_en(1); // 打开音频功放
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_PAUSE: // 正在暂停音乐
        ESP_LOGI(TAG, "AUDIO_PLAYER_REQUEST_PAUSE");
        pa_en(0); // 关闭音频功放
        break;
    default:
        break;
    }
}

// mp3播放器初始化
void mp3_player_init(void)
{
    // 获取文件信息
    file_iterator = file_iterator_new(SPIFFS_BASE);
    assert(file_iterator != NULL);

    // 初始化音频播放
    player_config.mute_fn = _audio_player_mute_fn;
    player_config.write_fn = _audio_player_write_fn;
    player_config.clk_set_fn = _audio_player_std_clock;
    player_config.priority = 6;
    player_config.coreID = 1;

    ESP_ERROR_CHECK(audio_player_new(player_config));
    ESP_ERROR_CHECK(audio_player_callback_register(_audio_player_callback, NULL));
}


// 按钮样式相关定义
typedef struct {
    lv_style_t style_bg;
    lv_style_t style_focus_no_outline;
} button_style_t;
static button_style_t g_btn_styles;

button_style_t *ui_button_styles(void)
{
    return &g_btn_styles;
}

// 按钮样式初始化
static void ui_button_style_init(void)
{
    /*Init the style for the default state*/
    lv_style_init(&g_btn_styles.style_focus_no_outline);
    lv_style_set_outline_width(&g_btn_styles.style_focus_no_outline, 0);

    lv_style_init(&g_btn_styles.style_bg);
    lv_style_set_bg_opa(&g_btn_styles.style_bg, LV_OPA_100);
    lv_style_set_bg_color(&g_btn_styles.style_bg, lv_color_make(255, 255, 255));
    lv_style_set_shadow_width(&g_btn_styles.style_bg, 0);
}

// 播放暂停按钮 事件处理函数
static void btn_play_pause_cb(lv_event_t *event)
{
    lv_obj_t *btn = lv_event_get_target(event);
    lv_obj_t *lab = (lv_obj_t *) btn->user_data;

    audio_player_state_t state = audio_player_get_state();
    printf("state=%d\n", state);
    if(state == AUDIO_PLAYER_STATE_IDLE){
        lvgl_port_lock(0);
        lv_label_set_text_static(lab, LV_SYMBOL_PAUSE);
        lvgl_port_unlock();
        int index = file_iterator_get_index(file_iterator);
        ESP_LOGI(TAG, "playing index '%d'", index);
        play_index(index);
    }else if (state == AUDIO_PLAYER_STATE_PAUSE) {
        lvgl_port_lock(0);
        lv_label_set_text_static(lab, LV_SYMBOL_PAUSE);
        lvgl_port_unlock();
        audio_player_resume();
    } else if (state == AUDIO_PLAYER_STATE_PLAYING) {
        lvgl_port_lock(0);
        lv_label_set_text_static(lab, LV_SYMBOL_PLAY);
        lvgl_port_unlock();
        audio_player_pause();
    }
}

// 上一首 下一首 按键事件处理函数
static void btn_prev_next_cb(lv_event_t *event)
{
    bool is_next = (bool) event->user_data;

    if (is_next) {
        ESP_LOGI(TAG, "btn next");
        file_iterator_next(file_iterator);
    } else {
        ESP_LOGI(TAG, "btn prev");
        file_iterator_prev(file_iterator);
    }
    // 修改当前的音乐名称
    int index = file_iterator_get_index(file_iterator);
    lvgl_port_lock(0);
    lv_dropdown_set_selected(music_list, index);
    // lv_obj_t *label_title = (lv_obj_t *) music_list->user_data;
    // lv_label_set_text_static(label_title, file_iterator_get_name_from_index(file_iterator, index));
    lvgl_port_unlock();
    // 执行音乐事件
    audio_player_state_t state = audio_player_get_state();
    printf("prev_next_state=%d\n", state);
    if (state == AUDIO_PLAYER_STATE_IDLE) { 
        // Nothing to do
    }else if (state == AUDIO_PLAYER_STATE_PAUSE){ // 如果当前正在暂停歌曲
        ESP_LOGI(TAG, "playing index '%d'", index);
        play_index(index);
        audio_player_pause();
    } else if (state == AUDIO_PLAYER_STATE_PLAYING) { // 如果当前正在播放歌曲
        // 播放歌曲
        ESP_LOGI(TAG, "playing index '%d'", index);
        play_index(index);
    }
}

// 音量调节滑动条 事件处理函数
static void volume_slider_cb(lv_event_t *event)
{
    lv_obj_t *slider = lv_event_get_target(event);
    int volume = lv_slider_get_value(slider); // 获取slider的值
    bsp_codec_volume_set(volume, NULL); // 设置声音大小
    g_sys_volume = volume; // 把声音赋值给g_sys_volume保存
    ESP_LOGI(TAG, "volume '%d'", volume);
}

// 音乐列表 点击事件处理函数
static void music_list_cb(lv_event_t *event)
{   
    uint16_t index = lv_dropdown_get_selected(music_list);
    ESP_LOGI(TAG, "switching index to '%d'", index);
    file_iterator_set_index(file_iterator, index);
    
    audio_player_state_t state = audio_player_get_state();
    if (state == AUDIO_PLAYER_STATE_PAUSE){ // 如果当前正在暂停歌曲
        play_index(index);
        audio_player_pause();
    } else if (state == AUDIO_PLAYER_STATE_PLAYING) { // 如果当前正在播放歌曲
        play_index(index);
    }
}

// 音乐名称加入列表
static void build_file_list(lv_obj_t *music_list)
{
    lvgl_port_lock(0);
    lv_dropdown_clear_options(music_list);
    lvgl_port_unlock();

    for(size_t i = 0; i<file_iterator->count; i++)
    {
        const char *file_name = file_iterator_get_name_from_index(file_iterator, i);
        if (NULL != file_name) {
            lvgl_port_lock(0);
            lv_dropdown_add_option(music_list, file_name, i); // 添加音乐名称到列表中
            lvgl_port_unlock();
        }
    }
    lvgl_port_lock(0);
    lv_dropdown_set_selected(music_list, 0); // 选择列表中的第一个
    lvgl_port_unlock();
}

// 播放器界面初始化
void music_ui(void)
{
    lvgl_port_lock(0);

    ui_button_style_init();// 初始化按键风格

    /* 创建播放暂停控制按键 */
    btn_play_pause = lv_btn_create(icon_in_obj);
    lv_obj_align(btn_play_pause, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_size(btn_play_pause, 50, 50);
    lv_obj_set_style_radius(btn_play_pause, 25, LV_STATE_DEFAULT);
    lv_obj_add_flag(btn_play_pause, LV_OBJ_FLAG_CHECKABLE);

    lv_obj_add_style(btn_play_pause, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_play_pause, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);

    label_play_pause = lv_label_create(btn_play_pause);

    lv_label_set_text_static(label_play_pause, LV_SYMBOL_PLAY);
    lv_obj_center(label_play_pause);

    lv_obj_set_user_data(btn_play_pause, (void *) label_play_pause);
    lv_obj_add_event_cb(btn_play_pause, btn_play_pause_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* 创建上一首控制按键 */
    lv_obj_t *btn_play_prev = lv_btn_create(icon_in_obj);
    lv_obj_set_size(btn_play_prev, 50, 50);
    lv_obj_set_style_radius(btn_play_prev, 25, LV_STATE_DEFAULT);
    lv_obj_clear_flag(btn_play_prev, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_align_to(btn_play_prev, btn_play_pause, LV_ALIGN_OUT_LEFT_MID, -40, 0); 

    lv_obj_add_style(btn_play_prev, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_play_prev, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);
    lv_obj_add_style(btn_play_prev, &ui_button_styles()->style_bg, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_play_prev, &ui_button_styles()->style_bg, LV_STATE_FOCUSED);
    lv_obj_add_style(btn_play_prev, &ui_button_styles()->style_bg, LV_STATE_DEFAULT);

    lv_obj_t *label_prev = lv_label_create(btn_play_prev);
    lv_label_set_text_static(label_prev, LV_SYMBOL_PREV);
    lv_obj_set_style_text_font(label_prev, &lv_font_montserrat_24, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_prev, lv_color_make(0, 0, 0), LV_STATE_DEFAULT);
    lv_obj_center(label_prev);
    lv_obj_set_user_data(btn_play_prev, (void *) label_prev);
    lv_obj_add_event_cb(btn_play_prev, btn_prev_next_cb, LV_EVENT_CLICKED, (void *) false);

    /* 创建下一首控制按键 */
    lv_obj_t *btn_play_next = lv_btn_create(icon_in_obj);
    lv_obj_set_size(btn_play_next, 50, 50);
    lv_obj_set_style_radius(btn_play_next, 25, LV_STATE_DEFAULT);
    lv_obj_clear_flag(btn_play_next, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_align_to(btn_play_next, btn_play_pause, LV_ALIGN_OUT_RIGHT_MID, 40, 0);

    lv_obj_add_style(btn_play_next, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_play_next, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);
    lv_obj_add_style(btn_play_next, &ui_button_styles()->style_bg, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_play_next, &ui_button_styles()->style_bg, LV_STATE_FOCUSED);
    lv_obj_add_style(btn_play_next, &ui_button_styles()->style_bg, LV_STATE_DEFAULT);

    lv_obj_t *label_next = lv_label_create(btn_play_next);
    lv_label_set_text_static(label_next, LV_SYMBOL_NEXT);
    lv_obj_set_style_text_font(label_next, &lv_font_montserrat_24, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_next, lv_color_make(0, 0, 0), LV_STATE_DEFAULT);
    lv_obj_center(label_next);
    lv_obj_set_user_data(btn_play_next, (void *) label_next);
    lv_obj_add_event_cb(btn_play_next, btn_prev_next_cb, LV_EVENT_CLICKED, (void *) true);

    /* 创建声音调节滑动条 */
    volume_slider = lv_slider_create(icon_in_obj);
    lv_obj_set_size(volume_slider, 200, 10);
    lv_obj_set_ext_click_area(volume_slider, 15);
    lv_obj_align(volume_slider, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_slider_set_range(volume_slider, 0, 100);
    lv_slider_set_value(volume_slider, g_sys_volume, LV_ANIM_ON);
    lv_obj_add_event_cb(volume_slider, volume_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *lab_vol_min = lv_label_create(icon_in_obj);
    lv_label_set_text_static(lab_vol_min, LV_SYMBOL_VOLUME_MID);
    lv_obj_set_style_text_font(lab_vol_min, &lv_font_montserrat_20, LV_STATE_DEFAULT);
    lv_obj_align_to(lab_vol_min, volume_slider, LV_ALIGN_OUT_LEFT_MID, -10, 0);

    lv_obj_t *lab_vol_max = lv_label_create(icon_in_obj);
    lv_label_set_text_static(lab_vol_max, LV_SYMBOL_VOLUME_MAX);
    lv_obj_set_style_text_font(lab_vol_max, &lv_font_montserrat_20, LV_STATE_DEFAULT);
    lv_obj_align_to(lab_vol_max, volume_slider, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    /* 创建音乐列表 */ 
    music_list = lv_dropdown_create(icon_in_obj);
    lv_dropdown_clear_options(music_list);
    lv_dropdown_set_options_static(music_list, "扫描中...");
    lv_obj_set_style_text_font(music_list, &font_alipuhui20, LV_STATE_ANY);
    lv_obj_set_width(music_list, 200);
    lv_obj_align(music_list, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_add_event_cb(music_list, music_list_cb, LV_EVENT_VALUE_CHANGED, NULL);

    build_file_list(music_list);

    lvgl_port_unlock();
}

// 返回主界面按钮事件处理函数
static void btn_music_back_cb(lv_event_t * e)
{
    lv_obj_del(icon_in_obj); 
    audio_player_delete();
    icon_flag = 0;
}

// 进入音乐播放应用
static void music_event_handler(lv_event_t * e)
{
    // 初始化mp3播放器
    mp3_player_init();
    // 创建一个界面对象
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_radius(&style, 10);  
    lv_style_set_bg_opa( &style, LV_OPA_COVER);
    lv_style_set_bg_color(&style, lv_color_hex(0xffffff));
    lv_style_set_border_width(&style, 0);
    lv_style_set_pad_all(&style, 0);
    lv_style_set_width(&style, 320);  
    lv_style_set_height(&style, 240); 

    icon_in_obj = lv_obj_create(lv_scr_act());
    lv_obj_add_style(icon_in_obj, &style, 0);

    // 创建标题背景
    lv_obj_t *music_title = lv_obj_create(icon_in_obj);
    lv_obj_set_size(music_title, 320, 40);
    lv_obj_set_style_pad_all(music_title, 0, 0);  // 设置间隙
    lv_obj_align(music_title, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(music_title, lv_color_hex(0xf87c30), 0);
    // 显示标题
    music_title_label = lv_label_create(music_title);
    lv_label_set_text(music_title_label, "Music");
    lv_obj_set_style_text_color(music_title_label, lv_color_hex(0xffffff), 0); 
    lv_obj_set_style_text_font(music_title_label, &font_alipuhui20, 0);
    lv_obj_align(music_title_label, LV_ALIGN_CENTER, 0, 0);
    // 创建后退按钮
    btn_music_back = lv_btn_create(music_title);
    lv_obj_align(btn_music_back, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_size(btn_music_back, 60, 30);
    lv_obj_set_style_border_width(btn_music_back, 0, 0); // 设置边框宽度
    lv_obj_set_style_pad_all(btn_music_back, 0, 0);  // 设置间隙
    lv_obj_set_style_bg_opa(btn_music_back, LV_OPA_TRANSP, LV_PART_MAIN); // 背景透明
    lv_obj_set_style_shadow_opa(btn_music_back, LV_OPA_TRANSP, LV_PART_MAIN); // 阴影透明
    lv_obj_add_event_cb(btn_music_back, btn_music_back_cb, LV_EVENT_CLICKED, NULL); // 添加按键处理函数

    lv_obj_t *label_back = lv_label_create(btn_music_back); 
    lv_label_set_text(label_back, LV_SYMBOL_LEFT);  // 按键上显示左箭头符号
    lv_obj_set_style_text_font(label_back, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label_back, lv_color_hex(0xffffff), 0); 
    lv_obj_align(label_back, LV_ALIGN_CENTER, -10, 0);

    icon_flag = 1; // 标记已经进入第二个应用

    music_ui(); // 音乐播放器界面
}

/******************************** 第2个图标 天气预报 应用程序*****************************************************************************/

#define WIFI_CONNECTED_BIT          BIT0
#define WIFI_GET_SNTP_BIT           BIT1
#define WIFI_GET_DAILYWEATHER_BIT   BIT2
#define WIFI_GET_RTWEATHER_BIT      BIT3
#define WIFI_GET_WEATHER_BIT        BIT4

#define MAX_HTTP_OUTPUT_BUFFER      2048

#define QWEATHER_DAILY_URL   "https://m63aank2w7.re.qweatherapi.com/v7/weather/3d?&location=101190104&key=ef08fe8570eb4a6591c88addf1af8db0"
#define QWEATHER_NOW_URL     "https://m63aank2w7.re.qweatherapi.com/v7/weather/now?location=101190104&key=ef08fe8570eb4a6591c88addf1af8db0"
#define QAIR_NOW_URL         "https://m63aank2w7.re.qweatherapi.com/v7/air/now?&location=101190104&key=ef08fe8570eb4a6591c88addf1af8db0"

lv_obj_t *weather_main_page;

lv_obj_t * label_wifi;
lv_obj_t * label_sntp;
lv_obj_t * label_weather;
lv_obj_t * qweather_icon_label;
lv_obj_t * qweather_temp_label;
lv_obj_t * qweather_text_label;
lv_obj_t * qair_level_obj;
lv_obj_t * qair_level_label;
lv_obj_t * led_time_label;
lv_obj_t * week_label;
lv_obj_t * sunset_label;
lv_obj_t *indoor_temp_label;
lv_obj_t *indoor_humi_label;
lv_obj_t *outdoor_temp_label;
lv_obj_t *outdoor_humi_label;
lv_obj_t * date_label;

float temp, humi;

int reset_flag;
int th_update_flag;
int qwnow_update_flag;
int qair_update_flag;
int qwdaily_update_flag;

static EventGroupHandle_t weather_event_group;

time_t now;
struct tm timeinfo;

int temp_value, humi_value;

int qwnow_temp;             // 实时天气温度
int qwnow_humi;             // 实时天气湿度
int qwnow_icon;             // 实时天气图标
char qwnow_text[32];        // 实时天气状态

int qwdaily_tempMax;        // 当天最高温度
int qwdaily_tempMin;        // 当天最低温度
char qwdaily_sunrise[10];   // 当天日出时间
char qwdaily_sunset[10];    // 当天日落时间

int qanow_level;            // 实时空气质量等级

// 返回主界面按钮事件处理函数
static void btn_th_backmain_cb(lv_event_t * e)
{
    ESP_LOGI(TAG, "btn_backmain Clicked");
    lv_obj_del(icon_in_obj);
    icon_flag = 0;
}
// 显示天气图标
void lv_qweather_icon_show(void)
{
    switch (qwnow_icon)
    {
        case 100: lv_label_set_text(qweather_icon_label, "\xEF\x84\x81"); strcpy(qwnow_text, "晴"); break;
        case 101: lv_label_set_text(qweather_icon_label, "\xEF\x84\x82"); strcpy(qwnow_text, "多云"); break;
        case 102: lv_label_set_text(qweather_icon_label, "\xEF\x84\x83"); strcpy(qwnow_text, "少云"); break;
        case 103: lv_label_set_text(qweather_icon_label, "\xEF\x84\x84"); strcpy(qwnow_text, "晴间多云"); break;
        case 104: lv_label_set_text(qweather_icon_label, "\xEF\x84\x85"); strcpy(qwnow_text, "阴"); break;
        case 150: lv_label_set_text(qweather_icon_label, "\xEF\x84\x86"); strcpy(qwnow_text, "晴"); break;
        case 151: lv_label_set_text(qweather_icon_label, "\xEF\x84\x87"); strcpy(qwnow_text, "多云"); break;
        case 152: lv_label_set_text(qweather_icon_label, "\xEF\x84\x88"); strcpy(qwnow_text, "少云"); break;
        case 153: lv_label_set_text(qweather_icon_label, "\xEF\x84\x89"); strcpy(qwnow_text, "晴间多云"); break;
        case 300: lv_label_set_text(qweather_icon_label, "\xEF\x84\x8A"); strcpy(qwnow_text, "阵雨"); break;
        case 301: lv_label_set_text(qweather_icon_label, "\xEF\x84\x8B"); strcpy(qwnow_text, "强阵雨"); break;
        case 302: lv_label_set_text(qweather_icon_label, "\xEF\x84\x8C"); strcpy(qwnow_text, "雷阵雨"); break;
        case 303: lv_label_set_text(qweather_icon_label, "\xEF\x84\x8D"); strcpy(qwnow_text, "强雷阵雨"); break;
        case 304: lv_label_set_text(qweather_icon_label, "\xEF\x84\x8E"); strcpy(qwnow_text, "雷阵雨伴有冰雹"); break;
        case 305: lv_label_set_text(qweather_icon_label, "\xEF\x84\x8F"); strcpy(qwnow_text, "小雨"); break;
        case 306: lv_label_set_text(qweather_icon_label, "\xEF\x84\x90"); strcpy(qwnow_text, "中雨"); break;
        case 307: lv_label_set_text(qweather_icon_label, "\xEF\x84\x91"); strcpy(qwnow_text, "大雨"); break;
        case 308: lv_label_set_text(qweather_icon_label, "\xEF\x84\x92"); strcpy(qwnow_text, "极端降雨"); break;
        case 309: lv_label_set_text(qweather_icon_label, "\xEF\x84\x93"); strcpy(qwnow_text, "毛毛雨"); break;
        case 310: lv_label_set_text(qweather_icon_label, "\xEF\x84\x94"); strcpy(qwnow_text, "暴雨"); break;
        case 311: lv_label_set_text(qweather_icon_label, "\xEF\x84\x95"); strcpy(qwnow_text, "大暴雨"); break;
        case 312: lv_label_set_text(qweather_icon_label, "\xEF\x84\x96"); strcpy(qwnow_text, "特大暴雨"); break;
        case 313: lv_label_set_text(qweather_icon_label, "\xEF\x84\x97"); strcpy(qwnow_text, "冻雨"); break;
        case 314: lv_label_set_text(qweather_icon_label, "\xEF\x84\x98"); strcpy(qwnow_text, "小到中雨"); break;
        case 315: lv_label_set_text(qweather_icon_label, "\xEF\x84\x99"); strcpy(qwnow_text, "中到大雨"); break;
        case 316: lv_label_set_text(qweather_icon_label, "\xEF\x84\x9A"); strcpy(qwnow_text, "大到暴雨"); break;
        case 317: lv_label_set_text(qweather_icon_label, "\xEF\x84\x9B"); strcpy(qwnow_text, "暴雨到大暴雨"); break;
        case 318: lv_label_set_text(qweather_icon_label, "\xEF\x84\x9C"); strcpy(qwnow_text, "大暴雨到特大暴雨"); break;
        case 350: lv_label_set_text(qweather_icon_label, "\xEF\x84\x9D"); strcpy(qwnow_text, "阵雨"); break;
        case 351: lv_label_set_text(qweather_icon_label, "\xEF\x84\x9E"); strcpy(qwnow_text, "强阵雨"); break;
        case 399: lv_label_set_text(qweather_icon_label, "\xEF\x84\x9F"); strcpy(qwnow_text, "雨"); break;
        case 400: lv_label_set_text(qweather_icon_label, "\xEF\x84\xA0"); strcpy(qwnow_text, "小雪"); break;
        case 401: lv_label_set_text(qweather_icon_label, "\xEF\x84\xA1"); strcpy(qwnow_text, "中雪"); break;
        case 402: lv_label_set_text(qweather_icon_label, "\xEF\x84\xA2"); strcpy(qwnow_text, "大雪"); break;
        case 403: lv_label_set_text(qweather_icon_label, "\xEF\x84\xA3"); strcpy(qwnow_text, "暴雪"); break;
        case 404: lv_label_set_text(qweather_icon_label, "\xEF\x84\xA4"); strcpy(qwnow_text, "雨夹雪"); break;
        case 405: lv_label_set_text(qweather_icon_label, "\xEF\x84\xA5"); strcpy(qwnow_text, "雨雪天气"); break;
        case 406: lv_label_set_text(qweather_icon_label, "\xEF\x84\xA6"); strcpy(qwnow_text, "阵雨夹雪"); break;
        case 407: lv_label_set_text(qweather_icon_label, "\xEF\x84\xA7"); strcpy(qwnow_text, "阵雪"); break;
        case 408: lv_label_set_text(qweather_icon_label, "\xEF\x84\xA8"); strcpy(qwnow_text, "小到中雪"); break;
        case 409: lv_label_set_text(qweather_icon_label, "\xEF\x84\xA9"); strcpy(qwnow_text, "中到大雪"); break;
        case 410: lv_label_set_text(qweather_icon_label, "\xEF\x84\xAA"); strcpy(qwnow_text, "大到暴雪"); break;
        case 456: lv_label_set_text(qweather_icon_label, "\xEF\x84\xAB"); strcpy(qwnow_text, "阵雨夹雪"); break;
        case 457: lv_label_set_text(qweather_icon_label, "\xEF\x84\xAC"); strcpy(qwnow_text, "阵雪"); break;
        case 499: lv_label_set_text(qweather_icon_label, "\xEF\x84\xAD"); strcpy(qwnow_text, "雪"); break;
        case 500: lv_label_set_text(qweather_icon_label, "\xEF\x84\xAE"); strcpy(qwnow_text, "薄雾"); break;
        case 501: lv_label_set_text(qweather_icon_label, "\xEF\x84\xAF"); strcpy(qwnow_text, "雾"); break;
        case 502: lv_label_set_text(qweather_icon_label, "\xEF\x84\xB0"); strcpy(qwnow_text, "霾"); break;
        case 503: lv_label_set_text(qweather_icon_label, "\xEF\x84\xB1"); strcpy(qwnow_text, "扬沙"); break;
        case 504: lv_label_set_text(qweather_icon_label, "\xEF\x84\xB2"); strcpy(qwnow_text, "浮尘"); break;
        case 507: lv_label_set_text(qweather_icon_label, "\xEF\x84\xB3"); strcpy(qwnow_text, "沙尘暴"); break;
        case 508: lv_label_set_text(qweather_icon_label, "\xEF\x84\xB4"); strcpy(qwnow_text, "强沙尘暴"); break;
        case 509: lv_label_set_text(qweather_icon_label, "\xEF\x84\xB5"); strcpy(qwnow_text, "浓雾"); break;
        case 510: lv_label_set_text(qweather_icon_label, "\xEF\x84\xB6"); strcpy(qwnow_text, "强浓雾"); break;
        case 511: lv_label_set_text(qweather_icon_label, "\xEF\x84\xB7"); strcpy(qwnow_text, "中度霾"); break;
        case 512: lv_label_set_text(qweather_icon_label, "\xEF\x84\xB8"); strcpy(qwnow_text, "重度霾"); break;
        case 513: lv_label_set_text(qweather_icon_label, "\xEF\x84\xB9"); strcpy(qwnow_text, "严重霾"); break;
        case 514: lv_label_set_text(qweather_icon_label, "\xEF\x84\xBA"); strcpy(qwnow_text, "大雾"); break;
        case 515: lv_label_set_text(qweather_icon_label, "\xEF\x84\xBB"); strcpy(qwnow_text, "特强浓雾"); break;
        case 900: lv_label_set_text(qweather_icon_label, "\xEF\x85\x84"); strcpy(qwnow_text, "热"); break;
        case 901: lv_label_set_text(qweather_icon_label, "\xEF\x85\x85"); strcpy(qwnow_text, "冷"); break;
    
        default:
            printf("ICON_CODE:%d\n", qwnow_icon);
            lv_label_set_text(qweather_icon_label, "\xEF\x85\x86");
            strcpy(qwnow_text, "未知天气");
            break;
    }
}

// 显示空气质量
void lv_qair_level_show(void)
{
    switch (qanow_level)
    {
        case 1: 
            lv_label_set_text(qair_level_label, "优"); 
            lv_obj_set_style_bg_color(qair_level_obj, lv_palette_main(LV_PALETTE_GREEN), 0); 
            lv_obj_set_style_text_color(qair_level_label, lv_color_hex(0xFFFFFF), 0);
            break;
        case 2: 
            lv_label_set_text(qair_level_label, "良"); 
            lv_obj_set_style_bg_color(qair_level_obj, lv_palette_main(LV_PALETTE_YELLOW), 0); 
            lv_obj_set_style_text_color(qair_level_label, lv_color_hex(0x000000), 0);
            break;
        case 3: 
            lv_label_set_text(qair_level_label, "轻");
            lv_obj_set_style_bg_color(qair_level_obj, lv_palette_main(LV_PALETTE_ORANGE), 0); 
            lv_obj_set_style_text_color(qair_level_label, lv_color_hex(0xFFFFFF), 0); 
            break;
        case 4: 
            lv_label_set_text(qair_level_label, "中"); 
            lv_obj_set_style_bg_color(qair_level_obj, lv_palette_main(LV_PALETTE_RED), 0); 
            lv_obj_set_style_text_color(qair_level_label, lv_color_hex(0xFFFFFF), 0);
            break; 
        case 5: 
            lv_label_set_text(qair_level_label, "重"); 
            lv_obj_set_style_bg_color(qair_level_obj, lv_palette_main(LV_PALETTE_PURPLE), 0); 
            lv_obj_set_style_text_color(qair_level_label, lv_color_hex(0xFFFFFF), 0);
            break;
        case 6: 
            lv_label_set_text(qair_level_label, "严"); 
            lv_obj_set_style_bg_color(qair_level_obj, lv_palette_main(LV_PALETTE_BROWN), 0); 
            lv_obj_set_style_text_color(qair_level_label, lv_color_hex(0xFFFFFF), 0);
            break;
        default: 
            lv_label_set_text(qair_level_label, "未"); 
            lv_obj_set_style_bg_color(qair_level_obj, lv_palette_main(LV_PALETTE_GREEN), 0); 
            lv_obj_set_style_text_color(qair_level_label, lv_color_hex(0xFFFFFF), 0);
            break;
    }
}

// 显示星期几
void lv_week_show(void)
{
    switch (timeinfo.tm_wday)
    {
        case 0: lv_label_set_text(week_label, "星期日"); break;
        case 1: lv_label_set_text(week_label, "星期一"); break;
        case 2: lv_label_set_text(week_label, "星期二"); break;
        case 3: lv_label_set_text(week_label, "星期三"); break;
        case 4: lv_label_set_text(week_label, "星期四"); break; 
        case 5: lv_label_set_text(week_label, "星期五"); break;
        case 6: lv_label_set_text(week_label, "星期六"); break;
        default: lv_label_set_text(week_label, "星期日"); break;
    }
}

// 显示weather界面
void lv_weather_page(void)
{
    lvgl_port_lock(0);
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0); // 修改背景为黑色
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_radius(&style, 10);  // 设置圆角半径
    lv_style_set_bg_opa( &style, LV_OPA_COVER );
    lv_style_set_bg_color(&style, lv_color_hex(0x00BFFF));
    lv_style_set_border_width(&style, 0);
    lv_style_set_pad_all(&style, 10);
    lv_style_set_width(&style, 320);  // 设置宽
    lv_style_set_height(&style, 240); // 设置高

    /*Create an object with the new style*/
    icon_in_obj = lv_obj_create(lv_scr_act());
    lv_obj_add_style(icon_in_obj, &style, 0);

    // 显示地理位置
    lv_obj_t * addr_label = lv_label_create(icon_in_obj);
    lv_obj_set_style_text_font(addr_label, &font_alipuhui20, 0);
    lv_label_set_text(addr_label, "南京市|江宁区");
    lv_obj_align_to(addr_label, icon_in_obj, LV_ALIGN_TOP_LEFT, 0, 0);

    // 显示年月日
    date_label = lv_label_create(icon_in_obj);
    lv_obj_set_style_text_font(date_label, &font_alipuhui20, 0);
    lv_label_set_text_fmt(date_label, "%d年%02d月%02d日", timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday);
    lv_obj_align_to(date_label, icon_in_obj, LV_ALIGN_TOP_RIGHT, 0, 0);

    // 显示分割线
    lv_obj_t * above_bar = lv_bar_create(icon_in_obj);
    lv_obj_set_size(above_bar, 300, 3);
    lv_obj_set_pos(above_bar, 0 , 30);
    lv_bar_set_value(above_bar, 100, LV_ANIM_OFF);

    // 显示天气图标
    qweather_icon_label = lv_label_create(icon_in_obj);
    lv_obj_set_style_text_font(qweather_icon_label, &font_qweather, 0);
    lv_obj_set_pos(qweather_icon_label, 0 , 40);
    lv_qweather_icon_show();

    // 显示空气质量
    static lv_style_t qair_level_style;
    lv_style_init(&qair_level_style);
    lv_style_set_radius(&qair_level_style, 10);  // 设置圆角半径
    lv_style_set_bg_color(&qair_level_style, lv_palette_main(LV_PALETTE_GREEN)); // 绿色
    lv_style_set_text_color(&qair_level_style, lv_color_hex(0xffffff)); // 白色
    lv_style_set_border_width(&qair_level_style, 0);
    lv_style_set_pad_all(&qair_level_style, 0);
    lv_style_set_width(&qair_level_style, 50);  // 设置宽
    lv_style_set_height(&qair_level_style, 26); // 设置高

    qair_level_obj = lv_obj_create(icon_in_obj);
    lv_obj_add_style(qair_level_obj, &qair_level_style, 0);
    lv_obj_align_to(qair_level_obj, qweather_icon_label, LV_ALIGN_OUT_RIGHT_TOP, 5, 0);

    qair_level_label = lv_label_create(qair_level_obj);
    lv_obj_set_style_text_font(qair_level_label, &font_alipuhui20, 0);
    lv_obj_align(qair_level_label, LV_ALIGN_CENTER, 0, 0);
    lv_qair_level_show();

    // 显示当天室外温度范围
    qweather_temp_label = lv_label_create(icon_in_obj);
    lv_obj_set_style_text_font(qweather_temp_label, &font_alipuhui20, 0);
    lv_label_set_text_fmt(qweather_temp_label, "%d~%d℃", qwdaily_tempMin, qwdaily_tempMax);
    lv_obj_align_to(qweather_temp_label, qweather_icon_label, LV_ALIGN_OUT_RIGHT_MID, 5, 5);

    // 显示当天天气图标代表的天气状况
    qweather_text_label = lv_label_create(icon_in_obj);
    lv_obj_set_style_text_font(qweather_text_label, &font_alipuhui20, 0);
    lv_label_set_long_mode(qweather_text_label, LV_LABEL_LONG_SCROLL_CIRCULAR);     /*Circular scroll*/
    lv_obj_set_width(qweather_text_label, 80);
    lv_label_set_text_fmt(qweather_text_label, "%s", qwnow_text);
    lv_obj_align_to(qweather_text_label, qweather_icon_label, LV_ALIGN_OUT_RIGHT_BOTTOM, 5, 0);
    
    // 显示时间  小时:分钟:秒钟
    led_time_label = lv_label_create(icon_in_obj);
    lv_obj_set_style_text_font(led_time_label, &font_led, 0);
    lv_label_set_text_fmt(led_time_label, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    lv_obj_set_pos(led_time_label, 142, 42);

    // 显示星期几
    week_label = lv_label_create(icon_in_obj);
    lv_obj_set_style_text_font(week_label, &font_alipuhui20, 0);
    lv_obj_align_to(week_label, led_time_label, LV_ALIGN_OUT_BOTTOM_RIGHT, -10, 6);
    lv_week_show();

    // 显示日落时间 
    sunset_label = lv_label_create(icon_in_obj);
    lv_obj_set_style_text_font(sunset_label, &font_alipuhui20, 0);
    lv_label_set_text_fmt(sunset_label, "日落 %s", qwdaily_sunset);
    lv_obj_set_pos(sunset_label, 200 , 103);

    // 显示分割线
    lv_obj_t * below_bar = lv_bar_create(icon_in_obj);
    lv_obj_set_size(below_bar, 300, 3);
    lv_obj_set_pos(below_bar, 0, 130);
    lv_bar_set_value(below_bar, 100, LV_ANIM_OFF);

    // 显示室外温湿度
    static lv_style_t outdoor_style;
    lv_style_init(&outdoor_style);
    lv_style_set_radius(&outdoor_style, 10);  // 设置圆角半径
    lv_style_set_bg_color(&outdoor_style, lv_color_hex(0xd8b010)); // 
    lv_style_set_text_color(&outdoor_style, lv_color_hex(0xffffff)); // 白色
    lv_style_set_border_width(&outdoor_style, 0);
    lv_style_set_pad_all(&outdoor_style, 5);
    lv_style_set_width(&outdoor_style, 100);  // 设置宽
    lv_style_set_height(&outdoor_style, 80); // 设置高

    lv_obj_t * outdoor_obj = lv_obj_create(icon_in_obj);
    lv_obj_add_style(outdoor_obj, &outdoor_style, 0);
    lv_obj_align(outdoor_obj, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lv_obj_t *outdoor_th_label = lv_label_create(outdoor_obj);
    lv_obj_set_style_text_font(outdoor_th_label, &font_alipuhui20, 0);
    lv_label_set_text(outdoor_th_label, "室外");
    lv_obj_align(outdoor_th_label, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *temp_symbol_label1 = lv_label_create(outdoor_obj);
    lv_obj_set_style_text_font(temp_symbol_label1, &font_myawesome, 0);
    lv_label_set_text(temp_symbol_label1, "\xEF\x8B\x88");  // 显示温度图标
    lv_obj_align(temp_symbol_label1, LV_ALIGN_LEFT_MID, 10, 0);

    outdoor_temp_label = lv_label_create(outdoor_obj);
    lv_obj_set_style_text_font(outdoor_temp_label, &font_alipuhui20, 0);
    lv_label_set_text_fmt(outdoor_temp_label, "%d℃", qwnow_temp);
    lv_obj_align_to(outdoor_temp_label, temp_symbol_label1, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    lv_obj_t *humi_symbol_label1 = lv_label_create(outdoor_obj);
    lv_obj_set_style_text_font(humi_symbol_label1, &font_myawesome, 0);
    lv_label_set_text(humi_symbol_label1, "\xEF\x81\x83");  // 显示湿度图标
    lv_obj_align(humi_symbol_label1, LV_ALIGN_BOTTOM_LEFT, 10, 0);

    outdoor_humi_label = lv_label_create(outdoor_obj);
    lv_obj_set_style_text_font(outdoor_humi_label, &font_alipuhui20, 0);
    lv_label_set_text_fmt(outdoor_humi_label, "%d%%", qwnow_humi);
    lv_obj_align_to(outdoor_humi_label, humi_symbol_label1, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    // 显示室内温湿度
    static lv_style_t indoor_style;
    lv_style_init(&indoor_style);
    lv_style_set_radius(&indoor_style, 10);  // 设置圆角半径
    lv_style_set_bg_color(&indoor_style, lv_color_hex(0xfe6464)); //
    lv_style_set_text_color(&indoor_style, lv_color_hex(0xffffff)); // 白色
    lv_style_set_border_width(&indoor_style, 0);
    lv_style_set_pad_all(&indoor_style, 5);
    lv_style_set_width(&indoor_style, 100);  // 设置宽
    lv_style_set_height(&indoor_style, 80); // 设置高

    lv_obj_t * indoor_obj = lv_obj_create(icon_in_obj);
    lv_obj_add_style(indoor_obj, &indoor_style, 0);
    lv_obj_align(indoor_obj, LV_ALIGN_BOTTOM_MID, 10, 0);

    lv_obj_t *indoor_th_label = lv_label_create(indoor_obj);
    lv_obj_set_style_text_font(indoor_th_label, &font_alipuhui20, 0);
    lv_label_set_text(indoor_th_label, "室内");
    lv_obj_align(indoor_th_label, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *temp_symbol_label2 = lv_label_create(indoor_obj);
    lv_obj_set_style_text_font(temp_symbol_label2, &font_myawesome, 0);
    lv_label_set_text(temp_symbol_label2, "\xEF\x8B\x88");  // 温度图标
    lv_obj_align(temp_symbol_label2, LV_ALIGN_LEFT_MID, 10, 0);

    indoor_temp_label = lv_label_create(indoor_obj);
    lv_obj_set_style_text_font(indoor_temp_label, &font_alipuhui20, 0);
    lv_label_set_text_fmt(indoor_temp_label, "%d℃", temp_value);
    lv_obj_align_to(indoor_temp_label, temp_symbol_label2, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    lv_obj_t *humi_symbol_label2 = lv_label_create(indoor_obj);
    lv_obj_set_style_text_font(humi_symbol_label2, &font_myawesome, 0);
    lv_label_set_text(humi_symbol_label2, "\xEF\x81\x83");  // 湿度图标
    lv_obj_align(humi_symbol_label2, LV_ALIGN_BOTTOM_LEFT, 10, 0);

    indoor_humi_label = lv_label_create(indoor_obj);
    lv_obj_set_style_text_font(indoor_humi_label, &font_alipuhui20, 0);
    lv_label_set_text_fmt(indoor_humi_label, "%d%%", humi_value);
    lv_obj_align_to(indoor_humi_label, humi_symbol_label2, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    // 显示返回
    static lv_style_t back_style;
    lv_style_init(&back_style);
    lv_style_set_radius(&back_style, 10);  // 设置圆角半径
    lv_style_set_bg_color(&back_style, lv_color_hex(0x3E2723)); //
    lv_style_set_text_color(&back_style, lv_color_hex(0xFFFFFF)); // 白色
    lv_style_set_text_font(&back_style, &lv_font_montserrat_28);
    lv_style_set_border_width(&back_style, 0);
    lv_style_set_pad_all(&back_style, 5);
    lv_style_set_width(&back_style, 100);  // 设置宽
    lv_style_set_height(&back_style, 80); // 设置高

    // 创建返回按钮
    lv_obj_t *btn_back_obj = lv_btn_create(icon_in_obj);
    lv_obj_add_style(btn_back_obj, &back_style, 0);
    lv_obj_set_size(btn_back_obj, 80, 80);
    lv_obj_align(btn_back_obj, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_add_event_cb(btn_back_obj, btn_th_backmain_cb, LV_EVENT_CLICKED, NULL); // 添加按键处理函数

    lv_obj_t *label_back_label = lv_label_create(btn_back_obj); 
    lv_label_set_text(label_back_label, "BACK");  // 按键上显示左箭头符号
    lv_obj_align(label_back_label, LV_ALIGN_CENTER, 0, 0);
    
    lvgl_port_unlock();
}
static void weather_page_task(void *pvParameters)
{
    lv_weather_page();
    while(1)
    {
        vTaskDelay(100);
    }
    vTaskDelete(NULL);
}

static void th_event_handler(lv_event_t * e)
{
    // xTaskCreate(wifi_connect_task, "wifi_connect_task", 8192, NULL, 5, NULL);   // 一次性任务   连接WIFI
    // xTaskCreate(get_time_task, "get_time_task", 8192, NULL, 5, NULL);           // 一次性任务   获取网络时间
    // xTaskCreate(get_dwather_task, "get_dwather_task", 8192, NULL, 5, NULL);     // 一次性任务   获取每日天气信息
    // xTaskCreate(get_rtweather_task, "get_rtweather_task", 8192, NULL, 5, NULL); // 一次性任务   获取实时天气信息
    // xTaskCreate(get_airq_task, "get_airq_task", 8192, NULL, 5, NULL);     
    xTaskCreate(weather_page_task, "weather_page_task", 8192, NULL, 5, NULL);         // 非一次性任务 主界面任务

}

/******************************** 主界面  ******************************/
void Main_Page(void)
{
    lvgl_port_lock(0);
    // 创建主界面基本对象
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xffffff), 0);

    // 设置主界面style
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_radius(&style, 10);  
    lv_style_set_bg_opa( &style, LV_OPA_COVER );
    lv_style_set_bg_color(&style, lv_color_hex(0x00BFFF));
    lv_style_set_bg_grad_color( &style, lv_color_hex( 0x00BF00 ) );
    lv_style_set_bg_grad_dir( &style, LV_GRAD_DIR_VER );
    lv_style_set_border_width(&style, 0);
    lv_style_set_pad_all(&style, 0);
    lv_style_set_width(&style, 320);  
    lv_style_set_height(&style, 240); 

    // 设置应用图标style
    static lv_style_t btn_style;
    lv_style_init(&btn_style);
    lv_style_set_radius(&btn_style, 16);  
    lv_style_set_bg_opa( &btn_style, LV_OPA_COVER );
    lv_style_set_text_color(&btn_style, lv_color_hex(0xffffff)); 
    lv_style_set_border_width(&btn_style, 0);
    lv_style_set_pad_all(&btn_style, 5);
    lv_style_set_width(&btn_style, 80);  
    lv_style_set_height(&btn_style, 80); 

    main_obj = lv_obj_create(lv_scr_act());
    lv_obj_add_style(main_obj, &style, 0);

    // 显示左上角欢迎语
    main_text_label = lv_label_create(main_obj);
    lv_obj_set_style_text_font(main_text_label, &font_alipuhui20, 0);
    lv_label_set_long_mode(main_text_label, LV_LABEL_LONG_SCROLL_CIRCULAR);     /*Circular scroll*/
    lv_obj_set_width(main_text_label, 280);
    lv_label_set_text(main_text_label, "A demo for studying...");
    lv_obj_align_to(main_text_label, main_obj, LV_ALIGN_TOP_LEFT, 8, 5);

    // 创建第1个应用图标--音乐
    lv_obj_t *icon1 = lv_btn_create(main_obj);
    lv_obj_add_style(icon1, &btn_style, 0);
    lv_obj_set_style_bg_color(icon1, lv_color_hex(0xf87c30), 0);
    lv_obj_set_pos(icon1, 15, 50);
    lv_obj_add_event_cb(icon1, music_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t * img1 = lv_img_create(icon1);
    LV_IMG_DECLARE(img_music_icon);
    lv_img_set_src(img1, &img_music_icon);
    lv_obj_align(img1, LV_ALIGN_CENTER, 0, 0);

    // 创建第2个应用图标--天气
    lv_obj_t *icon2 = lv_btn_create(main_obj);
    lv_obj_add_style(icon2, &btn_style, 0);
    lv_obj_set_style_bg_color(icon2, lv_color_hex(0x3F51B5), 0);
    lv_obj_set_pos(icon2, 120, 50);
    lv_obj_add_event_cb(icon2, th_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t * img2 = lv_img_create(icon2);
    LV_IMG_DECLARE(img_th_icon);
    lv_img_set_src(img2, &img_th_icon);
    lv_obj_align(img2, LV_ALIGN_CENTER, 0, 0);

    // 创建第3个应用图标-->摄像头检测人脸
    lv_obj_t *icon3 = lv_btn_create(main_obj);
    lv_obj_add_style(icon3, &btn_style, 0);
    lv_obj_set_style_bg_color(icon3, lv_color_hex(0xd8b010), 0);
    lv_obj_set_pos(icon3, 225, 50);
    // lv_obj_add_event_cb(icon3, camera_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t * img3 = lv_img_create(icon3);
    LV_IMG_DECLARE(img_camera_icon);
    lv_img_set_src(img3, &img_camera_icon);
    lv_obj_align(img3, LV_ALIGN_CENTER, 0, 0);
    
    // 创建第4个应用图标
    lv_obj_t *icon4 = lv_btn_create(main_obj);
    lv_obj_add_style(icon4, &btn_style, 0);
    lv_obj_set_style_bg_color(icon4, lv_color_hex(0x7E57C2), 0);
    lv_obj_set_pos(icon4, 15, 147);
    // lv_obj_add_event_cb(icon4, att_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t * img4 = lv_img_create(icon4);
    LV_IMG_DECLARE(img_spr_icon);
    lv_img_set_src(img4, &img_spr_icon);
    lv_obj_align(img4, LV_ALIGN_CENTER, 0, 0);

    // 创建第5个应用图标-->设置
    lv_obj_t *icon5 = lv_btn_create(main_obj);
    lv_obj_add_style(icon5, &btn_style, 0);
    lv_obj_set_style_bg_color(icon5, lv_color_hex(0x6D4C41), 0);
    lv_obj_set_pos(icon5, 120, 147);
    // lv_obj_add_event_cb(icon4, att_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t * img5 = lv_img_create(icon5);
    LV_IMG_DECLARE(img_set_icon);
    lv_img_set_src(img5, &img_set_icon);
    lv_obj_align(img5, LV_ALIGN_CENTER, 0, 0);

    
    // 创建第6个应用图标--ABOUT
    lv_obj_t *icon6 = lv_btn_create(main_obj);
    lv_obj_add_style(icon6, &btn_style, 0);
    lv_obj_set_style_bg_color(icon6, lv_color_hex(0x30a830), 0);
    lv_obj_set_pos(icon6, 225, 147);
    // lv_obj_add_event_cb(icon4, att_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t * img6 = lv_img_create(icon6);
    LV_IMG_DECLARE(img_btset_icon);
    lv_img_set_src(img6, &img_btset_icon);
    lv_obj_align(img6, LV_ALIGN_CENTER, 0, 0);

    lvgl_port_unlock();
}
