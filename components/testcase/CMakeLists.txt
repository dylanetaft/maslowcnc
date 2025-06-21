idf_component_register(SRCS "main.c" "machine.c" "peripherals_i2c.c" "adc.c" "external/AS5600_C/AS5600.c" "external/libTCA9548A/src/libTCA9548A.c"
                    PRIV_REQUIRES spi_flash esp_driver_gpio esp_driver_gptimer esp_adc esp_timer esp_driver_i2c
                    INCLUDE_DIRS "include" "external/AS5600_C" "external/libTCA9548A/src")
