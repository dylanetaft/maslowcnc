#ifndef MACHINE_H
#define MACHINE_H

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_attr.h"

//these pins tie to GPIO pin ids on the ESP32, not the physical pins
// CONFIG_ADC_CONTINUOUS_ISR_IRAM_SAFE, GPTIMER_ISR_IRAM_SAFE

#define PIN_BL_IN_1 37
#define PIN_BL_IN_2 36
#define PIN_BR_IN_1 9
#define PIN_BR_IN_2 3
#define PIN_TR_IN_1 42
#define PIN_TR_IN_2 41
#define PIN_TL_IN_1 45
#define PIN_TL_IN_2 21

#define PIN_TRINAMIC_TX 1

#define PIN_ADC_BL 8
#define PIN_ADC_BR 7
#define PIN_ADC_TR 6
#define PIN_ADC_TL 18

#define PIN_MZ_STEP 15
#define PIN_MZ_DIRECTION 16

#define PIN_COOLING_FAN 47


struct maslow_task_entry {
    void (*task_func)(); // Pointer to the task function
    struct maslow_task_entry *next; // Pointer to the next task in the list
    struct maslow_task_entry *prev; // Pointer to the previous task in the list
    bool is_linked; // Flag to indicate if the task is currently active
};


void maslow_task_append(struct maslow_task_entry *task);
void maslow_task_remove(struct maslow_task_entry *task);
void maslow_task_func_extend_belt();


// Initialize GPIO pins for the machine
// This function should be called in the setup phase of the application
void initialize_gpio_pins();
void initialize_task_timer();
void initialize_adc();


void maslow_initialize();

void enable_cooling_fan(bool enable);


extern volatile uint16_t adc_last_avg_readings[]; 

#endif
