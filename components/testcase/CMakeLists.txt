idf_component_register(SRCS "main.c" "machine.c" "adc.c" "external/AS5600_C/AS5600.c"
                    PRIV_REQUIRES spi_flash esp_driver_gpio esp_driver_gptimer esp_adc esp_timer
                    INCLUDE_DIRS "include" "external/AS5600_C")
