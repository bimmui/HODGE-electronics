if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/v2/main"
    )
endif()


idf_component_register(
    SRCS ${cpp_srcs} ${c_srcs} "lib.rs.cc"
    REQUIRES driver esp_driver_gpio esp_timer esp_event esp_driver_uart driver fatfs sd_card
    INCLUDE_DIRS ${hdrs}
    WHOLE_ARCHIVE
)
