target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/v2/main"
)

target_compile_definitions(pita-plate-firmware-v2
  PRIVATE MILLIS="(esp_timer_get_time() / 1000ULL)"
)


idf_component_register(
    SRCS ${cpp_srcs} ${c_srcs}
    REQUIRES driver esp_driver_gpio esp_timer esp_event esp_driver_uart driver fatfs sd_card
    INCLUDE_DIRS ${hdrs}
    WHOLE_ARCHIVE
)
