#include "bmi270_port.h"
#include "bmi2.h"
#include "i2c.h"


#define BMI270_ADDR 0x68


int8_t bmi270_i2c_write(uint8_t reg,
                        const uint8_t *data,
                        uint32_t len,
                        void *intf_ptr)
{
    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Write(
        &hi2c1,
        BMI270_ADDR<<1,
        reg,
        I2C_MEMADD_SIZE_8BIT,
        (uint8_t *)data,
        len,
        100
    );

    return (status == HAL_OK) ? BMI2_OK : BMI2_E_COM_FAIL;
}



int8_t bmi270_i2c_read(uint8_t reg,
                       uint8_t *data,
                       uint32_t len,
                       void *intf_ptr)
{
    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Read(
        &hi2c1,
        BMI270_ADDR<<1,
        reg,
        I2C_MEMADD_SIZE_8BIT,
        data,
        len,
        100
    );

    return (status == HAL_OK) ? BMI2_OK : BMI2_E_COM_FAIL;
}



/*
 * Microsecond delay required by the Bosch BMI270 driver.
 *
 * This used to be an empty software loop. GCC removes such a loop completely
 * at -Os (the Release preset): the whole function compiled down to a single
 * "bx lr", every driver delay became a no-op and bmi270_init() failed. The DWT
 * cycle counter cannot be optimised away and is independent of compiler flags.
 */
void bmi270_delay_us(uint32_t period, void *intf_ptr)
{
    uint32_t start;
    uint32_t cycles;

    (void)intf_ptr;

    if (period == 0U)
    {
        return;
    }

    /* Enable the DWT trace unit and start the free-running cycle counter. */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    /* 1 us equals SystemCoreClock / 1000000 core cycles (72 cycles at 72 MHz). */
    cycles = period * (SystemCoreClock / 1000000U);
    start = DWT->CYCCNT;

    /* Unsigned subtraction keeps this correct across the 32-bit counter wrap. */
    while ((DWT->CYCCNT - start) < cycles)
    {
        /* busy wait */
    }
}