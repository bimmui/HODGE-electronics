#idf_component_register(SRCS "cpp_pthread.cpp"
#                    INCLUDE_DIRS "."
#                    INCLUDE_DIRS "../")
#
#add_library(compress_rs STATIC IMPORTED GLOBAL)
#set_property(TARGET compress_rs PROPERTY IMPORTED_LOCATION "${COMPONENT_DIR}/../compress_rs/libcompress_rs.a")
#target_link_libraries(${COMPONENT_TARGET} PUBLIC compress_rs)

#set (APPLIBS_REQUIRES freertos driver nvs_flash spi_flash esp_hw_support esp_wifi esp_adc)\\

#if(NOT CMAKE_BUILD_EARLY_EXPANSION)
#
#set( Rust_CARGO_TARGET xtensa-esp32-none-elf)
#set(RUSTUP_TOOLCHAIN esp)
#include(FetchContent)
#
#FetchContent_Declare(
#    Corrosion
#    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
#    GIT_TAG v0.5 # Optionally specify a commit hash, version tag or branch here
#)
## Set any global configuration variables such as `Rust_TOOLCHAIN` before this line!
#FetchContent_MakeAvailable(Corrosion)
#
## Import targets defined in a package or workspace manifest `Cargo.toml` file
#corrosion_import_crate(MANIFEST_PATH ../../compress-rs/Cargo.toml CRATES compress-rs)
#corrosion_add_cxxbridge(cxxbridge-cpp CRATE compress_rs FILES lib.rs)
#
#ADD_CUSTOM_COMMAND(DEPENDS cxxbridge-cpp
#    OUTPUT compress_rs.h
#    COMMAND cp corrosion_generated/cxxbridge/cxxbridge-cpp/include/cxxbridge-cpp/lib.h compress_rs.h
#)
#
#endif() 

idf_component_register(SRCS "cpp_pthread.cpp"
                    INCLUDE_DIRS ".")
add_prebuilt_library(libcompress "${COMPONENT_DIR}/libcompress.a")
target_link_libraries(${COMPONENT_LIB} PUBLIC libcompress)