idf_component_register(SRCS "cpp_pthread.cpp" "lib.rs.cc" "gpio.cc"
                    INCLUDE_DIRS ".")
add_prebuilt_library(libcompress libcompress.a)
target_link_libraries(${COMPONENT_LIB} PUBLIC libcompress)