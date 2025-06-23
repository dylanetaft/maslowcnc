idf_component_register(SRCS "main.c" "machine.c" "i2c_beltsensor.c" "adc.c" "external/AS5600_C/AS5600.c" "external/libTCA9548A/src/libTCA9548A.c" "brushedmotor.c"
                    PRIV_REQUIRES spi_flash esp_driver_gpio esp_driver_gptimer esp_adc esp_timer esp_driver_i2c esp_driver_ledc
                    INCLUDE_DIRS "include" "external/AS5600_C" "external/libTCA9548A/src")
