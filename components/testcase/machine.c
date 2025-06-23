#include "include/machine.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "esp_timer.h"
#include "adc.h"
#include "i2c_beltsensor.h"
#include "brushedmotor.h"

struct maslow_task_entry *task_list_head = NULL; // Head of the task list




portMUX_TYPE maslow_task_mutex = portMUX_INITIALIZER_UNLOCKED;



struct maslow_task_entry maslow_task_extend_belt = {
    .task_func = &maslow_task_func_extend_belt, // Function to extend the belt
    .next = NULL,
    .prev = NULL,
    .is_linked = false
};


void maslow_initialize()
{
    initialize_gpio_pins(); // Initialize GPIO pins
    initialize_task_timer(); // Initialize the task timer
    adc_initialize(); // Initialize ADC for continuous reading
    initialize_belt_sensor(); // Initialize I2C peripherals
    initialize_brushedmotor(); // Initialize brushed motors
}

void maslow_task_func_extend_belt() {

}

void maslow_task_append(struct maslow_task_entry *task)
{
    portENTER_CRITICAL_ISR(&maslow_task_mutex);
    assert(task != NULL); // Ensure the task is not NULL
    assert(task->task_func != NULL); // Ensure the task function is set
    assert(!task->is_linked); // Ensure the task is not already linked

    if (task_list_head == NULL) {
        task->prev = NULL;
        task->next = NULL;
        task_list_head = task;
    } else {
        task->prev = NULL;
        task->next = task_list_head;
        task_list_head->prev = task;
        task_list_head = task;
    }
    task->is_linked = true; // Mark the task as linked
    portEXIT_CRITICAL_ISR(&maslow_task_mutex);
}

void maslow_task_remove(struct maslow_task_entry *task)
{
    portENTER_CRITICAL_ISR(&maslow_task_mutex);
    if (task_list_head == NULL) {
        return; // List is empty, nothing to remove
    }

    if (task_list_head == task) {
        // Removing the head of the list
        task_list_head = task->next;
        if (task_list_head != NULL) {
            task_list_head->prev = NULL;
        }
    } else {
        // Removing a task from the middle or end of the list
        if (task->prev != NULL) {
            task->prev->next = task->next;
        }
        if (task->next != NULL) {
            task->next->prev = task->prev;
        }
    }

    // Clear the removed task's pointers
    task->next = NULL;
    task->prev = NULL;
    task->is_linked = false; // Mark the task as unlinked
    portEXIT_CRITICAL_ISR(&maslow_task_mutex);
}

void initialize_gpio_pins()
{
     //fan enable
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_COOLING_FAN) | 
        (1ULL << PIN_MZ_STEP) |
        (1ULL << PIN_MZ_DIRECTION) | 
        (1ULL << PIN_TRINAMIC_TX),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(PIN_COOLING_FAN, 0);
    gpio_set_level(PIN_MZ_STEP, 0);
    gpio_set_level(PIN_MZ_DIRECTION, 0);
    gpio_set_level(PIN_TRINAMIC_TX, 0); // Set the Trinamic TX pin to low

    
    
}






static bool IRAM_ATTR task_timer_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    portENTER_CRITICAL_ISR(&maslow_task_mutex);
   

    // Optional: send the event data to other task by OS queue
    // Do not introduce complex logics in callbacks
    // Suggest dealing with event data in the main loop, instead of in this callback
    // xQueueSendFromISR(queue, &ele, &high_task_awoken);
    // return whether we need to yield at the end of ISR
    portEXIT_CRITICAL_ISR(&maslow_task_mutex);

    //return high_task_awoken == pdTRUE;
    return false; // No need to yield in this case
}



void initialize_task_timer()
{
    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1MHz, 1 tick=1us
        //where did the high priority uninterruptable timer go?

    };

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 1000, // 1000 µs = 1 ms
        .reload_count = 0, 
        .flags.auto_reload_on_alarm = true,
    };
    

    gptimer_event_callbacks_t cbs = {
        .on_alarm = task_timer_cb, // Set the ISR callback
    };
    
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_start(gptimer));


}

void enable_cooling_fan(bool enable)
{
    gpio_set_level(PIN_COOLING_FAN, enable); // Turn on or off the cooling fan
}
