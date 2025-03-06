set(srcs
"adxl375.cpp"
"icm20948.cpp"
"bmp581.cpp"
"nmea_parser.cpp"
"tmp_1075.cpp"
"main.cpp")

idf_component_register(SRCS "algebra.cpp" "altitude.cpp" "filters.cpp" "StateDetermination.cpp" "${srcs}"
                REQUIRES driver esp_driver_gpio esp_timer esp_event esp_driver_uart driver fatfs sd_card
                INCLUDE_DIRS include
                WHOLE_ARCHIVE)

target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--undefined=uxTopUsedPriority")