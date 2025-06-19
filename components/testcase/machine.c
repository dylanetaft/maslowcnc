#include "include/machine.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "esp_timer.h"


struct maslow_task_entry *task_list_head = NULL; // Head of the task list


adc_oneshot_unit_handle_t adc_handle[2] = {NULL, NULL}; // Array to hold ADC handles for each unit
volatile uint16_t adc_last_avg_readings[4] = {0}; // Array to hold the last readings from each ADC channel
esp_timer_handle_t adc_timer;


TaskHandle_t motor_adc_taskhandle = NULL; // Pointer to the created task handle
int64_t adc_last_read_time = 0; // Last time an ADC reading was taken
volatile bool adc_reading_in_progress = false; // Flag to indicate if an ADC reading is in progress

struct maslow_adc_unit_channel_mapping {
    uint8_t handle; // ADC unit (ADC_UNIT_1 or ADC_UNIT_2)
    adc_channel_t channel; // ADC channel (ADC_CHANNEL_0 to ADC_CHANNEL_9)  
    adc_unit_t unit; // ADC unit (ADC_UNIT_1 or ADC_UNIT_2)
};
struct maslow_adc_unit_channel_mapping adc_motor_pattern_config[] = {
    {0, ADC_CHANNEL_7, ADC_UNIT_1}, // BL
    {0, ADC_CHANNEL_6, ADC_UNIT_1}, // BR
    {0, ADC_CHANNEL_5, ADC_UNIT_1}, // TR
    {1, ADC_CHANNEL_7, ADC_UNIT_2}  // TL
};

portMUX_TYPE maslow_task_mutex = portMUX_INITIALIZER_UNLOCKED;



struct maslow_task_entry maslow_task_extend_belt = {
    .task_func = &maslow_task_func_extend_belt, // Function to extend the belt
    .next = NULL,
    .prev = NULL,
    .is_linked = false
};


static void IRAM_ATTR motor_adc_timer_callback (void* arg) {
    if ((esp_timer_get_time() - adc_last_read_time) < 1000) {
        return; // If the last reading was taken less than 100 microseconds ago, skip this callback
    }
    if (!adc_reading_in_progress) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(motor_adc_taskhandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } 
}

 //need to use a software timer
 //adc unit 2 cannot be used with continuous mode
static void motor_adc_task(void *arg)
{  
    int res;
    int val;

    while (1) {

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        adc_reading_in_progress = true;
        for (int chan = 0; chan < 4; chan++) {
            
            res = adc_oneshot_read(adc_handle[adc_motor_pattern_config[chan].handle],adc_motor_pattern_config[chan].channel, &val);
            if (res == ESP_OK) adc_last_avg_readings[0] = val; // Store the reading for BL       
        }
        adc_last_read_time = esp_timer_get_time();
        adc_reading_in_progress = false;
    }
   
}

void maslow_initialize()
{
    initialize_gpio_pins(); // Initialize GPIO pins
    initialize_task_timer(); // Initialize the task timer
    initialize_adc(); // Initialize ADC for continuous reading
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
        (1ULL << PIN_BL_IN_1) | 
        (1ULL << PIN_BL_IN_2) | 
        (1ULL << PIN_BR_IN_1) | 
        (1ULL << PIN_BR_IN_2) | 
        (1ULL << PIN_TR_IN_1) | 
        (1ULL << PIN_TR_IN_2) | 
        (1ULL << PIN_TL_IN_1) | 
        (1ULL << PIN_TL_IN_2) |
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
    gpio_set_level(PIN_BL_IN_1, 0);
    gpio_set_level(PIN_BL_IN_2, 0);
    gpio_set_level(PIN_BR_IN_1, 0);
    gpio_set_level(PIN_BR_IN_2, 0);
    gpio_set_level(PIN_TR_IN_1, 0);
    gpio_set_level(PIN_TR_IN_2, 0);
    gpio_set_level(PIN_TL_IN_1, 0);
    gpio_set_level(PIN_TL_IN_2, 0);
    gpio_set_level(PIN_TRINAMIC_TX, 0); // Set the Trinamic TX pin to low

    
    
}


void initialize_adc()
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };    
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle[0]));
    init_config.unit_id = ADC_UNIT_2;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle[1]));

    adc_oneshot_chan_cfg_t chan_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };




    
    //mapping of gpip->unit->channel found in datasheet for esp32 s3
    for (int chan = 0; chan < 4; chan++) {
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle[adc_motor_pattern_config[chan].handle],
                                                   adc_motor_pattern_config[chan].channel, &chan_config));
    }
    
    
    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &motor_adc_timer_callback,
            .name = "motor_adc_timer_callback"
    };
    // Create a task to read ADC values continuously
    xTaskCreate(motor_adc_task, "motor_adc_task", 2048, NULL, 5, &motor_adc_taskhandle);
    

    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &adc_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(adc_timer, 100)); // .1ms interval
    

    
    

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
