#include "adc.h"
#include "driver/gptimer.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "esp_timer.h"


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





void adc_initialize()
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