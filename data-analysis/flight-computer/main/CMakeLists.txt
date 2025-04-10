idf_component_register(SRCS "radio.cpp" "flight_computer.cpp" "lib.rs.cc"
                    INCLUDE_DIRS ".")
add_prebuilt_library(libcompress libcompress.a)
target_link_libraries(${COMPONENT_LIB} PUBLIC libcompress)