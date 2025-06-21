#include "peripherals_i2c.h"

#include "QWIICMUX.h"
#include "AS5600.h"
#include "adc.h"
#include "driver/gptimer.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "esp_timer.h"
#include <assert.h>

TaskHandle_t i2c_taskhandle = NULL; // Pointer to the created task handle
esp_timer_handle_t i2c_timer;

volatile bool i2c_task_busy = false;
volatile int64_t i2c_ask_last_execute_time = 0; // Last time an ADC reading was taken



i2c_master_bus_handle_t _bus_handle = NULL;
i2c_master_dev_handle_t _dev_handle_AS5600; = NULL;
i2c_master_dev_handle_t _dev_handle_QWIICMUX = NULL;
i2c_master_dev_handle_t *_current_dev_handle = NULL; // Pointer to the currently selected I2C device handle


struct AS5600_DEV _as5600_devices[4];

#define AS5600_ADDR = 0x36 // AS5600 default address
#define QWIICMUX_ADDR = 0x70 // QWIIC MUX default address

void i2c_start(uint8_t address) {
    _current_dev_handle = getI2CDevHandleByAddr(address);
    assert(_current_dev_handle != NULL);
    
}
void i2c_end() {
    _current_dev_handle = NULL; // Clear the current device handle
}
int i2c_writeBytes(uint8_t *data, uint8_t length) {
    assert(_current_dev_handle != NULL);
    esp_err_t retval = i2c_master_transmit(*_current_dev_handle, data,length, 5000);
    if (retval != ESP_OK) {
        return 1;
    }
    return 0; // Return 0 for success
}
int i2c_readBytes(uint8_t *data, uint8_t length) {
    assert(_current_dev_handle != NULL);
    esp_err_t retval = i2c_master_receive(*_current_dev_handle, data, length, 5000);
    if (retval != ESP_OK) {
        return 1; // Return 1 for error
    }
    return 0; 
}

static void i2c_task(void *arg)
{  
    int res;
    int val;

    while (1) {

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        i2c_task_busy = true;
        i2c_ask_last_execute_time = esp_timer_get_time();
        i2c_task_busy = false;
    }
   
}

static void IRAM_ATTR i2c_timer_callback (void* arg) {
    if ((esp_timer_get_time() - i2c_ask_last_execute_time) < 100000) {
        return; // If the last reading was taken less than 10ms ms ago, skip this callback
    }
    if (!i2c_task_busy) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(i2c_taskhandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } 
}


void initialize_i2c_devices() {
    
    i2c_master_bus_config_t i2c_mst_config = {0};

    i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.i2c_port = -1; //autoselect i2c port
    i2c_mst_config.scl_io_num = GPIO_NUM_4;
    i2c_mst_config.sda_io_num = GPIO_NUM_5;
    i2c_mst_config.glitch_ignore_cnt = 7;
    i2c_mst_config.flags.enable_internal_pullup = true;

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &_bus_handle));


    i2c_device_config_t dev_cfg = { 0 }; // zero everything
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = AS5600_ADDR; // AS5600 default address
    dev_cfg.scl_speed_hz = 100000; // 100kHz I2C speed


    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &_dev_handle_AS5600));

    dev_cfg = { 0 }; // zero everything
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = QWIICMUX_ADDR; //Quiic Mux default address
    dev_cfg.scl_speed_hz = 100000; // 100kHz I2C speed


    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &_dev_handle_QWIICMUX));


    struct AS5600_HAL as5600_hal{
            .digitalWrite = &gpio_set_level,
            .i2c_start = &i2c_start,
            .i2c_end = &i2c_end,
            .i2c_writeBytes = &i2c_writeBytes,
            .i2c_readBytes = &i2c_readBytes,
            .micros = &esp_timer_get_time
    };
    
    assert(QWIICMUX_begin((struct QWIICMUX_HAL){
        .i2c_start = &i2c_start,
        .i2c_end = &i2c_end,
        .i2c_writeBytes = &i2c_writeBytes,
        .i2c_readBytes = &i2c_readBytes
    })); // Initialize the QWIIC MUX

    for (int i = 0;i < 4;i++) {
        QUICKMUX_setPort(i);
        assert(AS5600_begin(&_as5600_devices[i], 255,as5600_hal)); //255 is sw direction pin, its grounded on maslow, assert it is connected

    }
  
}


i2c_master_dev_handle_t &getI2CDevHandleByAddr(uint8_t addr) {
    if (addr == AS5600_ADDR) { // AS5600 default addresses
        return &_dev_handle_AS5600;
    } else if (addr == QWIICMUX_ADDR) { // QWIIC MUX default address
        return &_dev_handle_QWIICMUX;
    } else {
        return NULL; // Invalid address
    }
}
void initialize_i2c_peripherals() {
    
    initialize_i2c_devices();

    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &i2c_timer_callback,
            .name = "i2c_timer_callback"
    };

    xTaskCreate(i2c_task, "i2c_task", 2048, NULL, 5, &motor_adc_taskhandle,1);
    

    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &i2c_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(i2c_timer, 1000)); // 1ms interval
    