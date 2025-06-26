idf_component_register(SRCS "main.c" "machine.c" "i2c_beltsensor.c" "adc.c" "external/I2C_HAL_drivers/AS5600/AS5600.c" "external/I2C_HAL_drivers/TCA954X/TCA954X.c" "brushedmotor.c" "networking.c"
                    PRIV_REQUIRES spi_flash esp_driver_gpio esp_driver_gptimer esp_adc esp_timer esp_driver_i2c esp_driver_ledc nvs_flash esp_wifi
                    INCLUDE_DIRS "include" "external/I2C_HAL_drivers")
