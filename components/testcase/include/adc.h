#ifndef ADC_H
#define ADC_H
#include <stdint.h>

extern volatile uint16_t adc_last_avg_readings[]; // Array to hold the last readings from each ADC channel
void adc_initialize();
#endif