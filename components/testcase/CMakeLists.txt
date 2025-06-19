idf_component_register(SRCS "main.c" "machine.c"
                    PRIV_REQUIRES spi_flash esp_driver_gpio esp_driver_gptimer esp_adc
                    INCLUDE_DIRS "include")
