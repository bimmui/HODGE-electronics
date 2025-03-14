#idf_component_register(SRCS "cpp_pthread.cpp"
#                    INCLUDE_DIRS "."
#                    INCLUDE_DIRS "../")
#
#add_library(compress_rs STATIC IMPORTED GLOBAL)
#set_property(TARGET compress_rs PROPERTY IMPORTED_LOCATION "${COMPONENT_DIR}/../compress_rs/libcompress_rs.a")
#target_link_libraries(${COMPONENT_TARGET} PUBLIC compress_rs)

#set (APPLIBS_REQUIRES freertos driver nvs_flash spi_flash esp_hw_support esp_wifi esp_adc)
idf_component_register(SRCS "cpp_pthread.cpp"
                    INCLUDE_DIRS ".")
add_prebuilt_library(cxxbridge-cpp "${COMPONENT_DIR}/lib/libcxxbridge-cpp.a")
target_link_libraries(${COMPONENT_LIB} PUBLIC cxxbridge-cpp)