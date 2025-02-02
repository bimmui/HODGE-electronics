idf_component_register( SRCS "bmp581.cpp" "main.cpp" "bmp581.h"
                        PRIV_REQUIRES spi_flash
                        REQUIRES driver
                        INCLUDE_DIRS ".")
