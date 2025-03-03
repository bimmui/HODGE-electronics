set(srcs
"AccelStepper.cpp"
"MultiStepper.cpp"
"LSM9DS1_ESP_IDF.cpp"
"main.cpp")

idf_component_register(SRCS "${srcs}"
                REQUIRES driver esp_driver_gpio esp_timer
                INCLUDE_DIRS include)

target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--undefined=uxTopUsedPriority")