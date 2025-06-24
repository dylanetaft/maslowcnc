#ifndef BRUSHEDMOTOR_H
#define BRUSHEDMOTOR_H
#include <stdint.h>

void initialize_brushedmotor();
void brushedmotor_set_speed(uint8_t motor_index, int8_t direction, uint8_t speed);
#endif