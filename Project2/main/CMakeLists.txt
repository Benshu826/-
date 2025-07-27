file(GLOB_RECURSE srcs *.c
                    assets/*.c
                    )
 
set(include_dirs 
    . 
    assets
    )
 
idf_component_register(
    SRCS ${srcs}
    SRCS "main.c" "app_ui.c" "assets/img_th_icon.c" "assets/img_spr_icon.c" "assets/img_set_icon.c"
    SRCS "assets/font_led.c" "assets/font_myawesome.c" "assets/font_qweather.c"
    INCLUDE_DIRS ${include_dirs}
    )
spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)