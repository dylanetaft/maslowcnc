#include <stdint.h>
#include "brushedmotor.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "driver/gpio.h"


#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (5) // Define the output GPIO
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT 
#define LEDC_FREQUENCY          (4000) // Frequency in Hertz. Set frequency at 4 kHz

#define PIN_BL_IN_1 37
#define PIN_BL_IN_2 36
#define PIN_BR_IN_1 9
#define PIN_BR_IN_2 3
#define PIN_TR_IN_1 42
#define PIN_TR_IN_2 41
#define PIN_TL_IN_1 45
#define PIN_TL_IN_2 21

const uint8_t _pins[8] = {
    PIN_BL_IN_1,
    PIN_BL_IN_2,
    PIN_BR_IN_1,
    PIN_BR_IN_2,
    PIN_TR_IN_1,
    PIN_TR_IN_2,
    PIN_TL_IN_1,
    PIN_TL_IN_2
};

const uint8_t _motor_to_channel[4] = {
    LEDC_CHANNEL_0, // BL
    LEDC_CHANNEL_1, // BR
    LEDC_CHANNEL_2, // TR
    LEDC_CHANNEL_3  // TL
};

int8_t _direction[4] = {0, 0, 0, 0}; // Direction for each motor


void zeroMotorPins(uint8_t motor_index) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << _pins[motor_index * 2]) | 
                        (1ULL << _pins[motor_index * 2 + 1]),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf); // Actually apply config
    gpio_set_level(_pins[motor_index * 2], 0);
    gpio_set_level(_pins[motor_index * 2 + 1], 0);
}

void initialize_brushedmotor() {
    

 for (int i = 0; i < 4; i++) zeroMotorPins(i); // Zero the motor pins

 ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 4 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD
    };

    for (int i = 0; i < 4; i++) {
        ledc_channel.channel = _motor_to_channel[i]; // Assign channel based on motor index
        ledc_channel.gpio_num = _pins[i * 2];
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
        //ESP_ERROR_CHECK(ledc_set_duty_and_update(LEDC_MODE, ledc_channel.channel, 0, 0)); // Set duty to 0%
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, ledc_channel.channel, 0));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, ledc_channel.channel));
    }

  
}

void brushedmotor_set_speed(uint8_t motor_index, int8_t direction, uint8_t speed) {
    if (motor_index >= 4) {
        return; // Invalid motor index
    }
    assert(direction == 0 || direction == 1); // Ensure direction is either 0 or 1
    ESP_ERROR_CHECK(ledc_stop(LEDC_MODE, _motor_to_channel[motor_index], 0)); // Stop the motor this cycle
    zeroMotorPins(motor_index); // Zero the motor pins

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .duty           = speed, 
        .hpoint         = 0,
        .channel        = _motor_to_channel[motor_index], // Assign channel based on motor index and direction
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD
    };
    if (direction == 0) {
        ledc_channel.gpio_num = _pins[motor_index * 2]; // Forward pin
    } else {
        ledc_channel.gpio_num = _pins[motor_index * 2 + 1]; // Backward pin
    }
    
    if (direction != _direction[motor_index]) ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    //ESP_ERROR_CHECK(ledc_set_duty_and_update(LEDC_MODE, ledc_channel.channel, speed, 0)); // Set duty cycle
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, ledc_channel.channel, speed));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, ledc_channel.channel));
    
    _direction[motor_index] = direction; // Update the direction



}

