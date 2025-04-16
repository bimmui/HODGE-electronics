# gathers all .cpp files in the current source directory.
file(GLOB_RECURSE cpp_srcs RELATIVE ${COMPONENT_DIR} "*.cpp")
file(GLOB_RECURSE c_srcs RELATIVE ${COMPONENT_DIR} "*.c")
list(FILTER cpp_srcs EXCLUDE REGEX ".*include/.*")
list(FILTER c_srcs EXCLUDE REGEX ".*include/.*")

# gathers all directories that are named "include"
file(GLOB_RECURSE hdrs RELATIVE ${COMPONENT_DIR} "*.h")
list(FILTER hdrs INCLUDE REGEX ".*/include/.*")
list(TRANSFORM hdrs REPLACE "(.*/)[^/]+$" "\\1")
list(REMOVE_DUPLICATES hdrs)

idf_component_register(
    SRCS ${cpp_srcs} ${c_srcs}
    REQUIRES driver esp_driver_gpio esp_timer esp_event esp_driver_uart driver fatfs sd_card
    INCLUDE_DIRS ${hdrs}
    WHOLE_ARCHIVE
)

target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--undefined=uxTopUsedPriority")
