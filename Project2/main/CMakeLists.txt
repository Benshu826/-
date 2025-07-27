file(GLOB_RECURSE srcs *.c
                    assets/*.c
                    )
 
set(include_dirs 
    . 
    assets
    )
 
idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${include_dirs}
    )
spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)