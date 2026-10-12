#ifndef __BMI270_PORT_H
#define __BMI270_PORT_H


#include "stdint.h"


int8_t bmi270_i2c_write(uint8_t reg,
                        const uint8_t *data,
                        uint32_t len,
                        void *intf_ptr);



int8_t bmi270_i2c_read(uint8_t reg,
                       uint8_t *data,
                       uint32_t len,
                       void *intf_ptr);



void bmi270_delay_us(uint32_t period, void *intf_ptr);



#endif