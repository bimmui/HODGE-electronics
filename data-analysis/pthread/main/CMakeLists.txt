if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    
endif()

idf_component_register(SRCS "cpp_pthread.cpp"
                    INCLUDE_DIRS ".")

add_custom_command(
    OUTPUT libcompress.a lib.rs.cc
    COMMAND ${COMPONENT_DIR}/../compile-rust.sh ${COMPONENT_DIR}/../../rust-lib/
)

add_custom_target(compress DEPENDS lib.rs.cc)

add_dependencies(${COMPONENT_LIB} compress)
add_prebuilt_library(libcompress libcompress.a)
target_link_libraries(${COMPONENT_LIB} PUBLIC libcompress)