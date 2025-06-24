#ifndef I2C_BELTSENSOR_H
#define I2C_BELTSENSOR_H
#include <stdint.h>

void initialize_belt_sensor();
volatile const int32_t *getBeltPositions();
#endif

