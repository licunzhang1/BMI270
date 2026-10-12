#include "main.h"
#include "gpio.h"
#include "i2c.h"
#include "usart.h"

#include "bmi270.h"
#include "bmi2.h"
#include "bmi270_port.h"

#include <stdio.h>



struct bmi2_dev bmi;

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                  RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 |
                                  RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}



void BMI270_Init(void)
{

    bmi.intf = BMI2_I2C_INTF;

    bmi.read = bmi270_i2c_read;

    bmi.write = bmi270_i2c_write;

    bmi.delay_us = bmi270_delay_us;


    bmi.intf_ptr = NULL;


    int8_t rslt;


    rslt = bmi270_init(&bmi);


    if(rslt==BMI2_OK)
    {
        printf("BMI270 OK\r\n");
    }
    else
    {
        printf("BMI270 ERROR\r\n");
    }



    /*
     * Zero-initialised on purpose: acc and gyr share one union, and only the
     * accelerometer branch is filled in by bmi2_get_sensor_config() below.
     * Without this, cfg.gyr.noise_perf (offset 5, past the end of the accel
     * struct) is read from uninitialised stack, so the gyroscope would land in
     * power-optimised or performance mode at random from one boot to the next.
     */
    struct bmi2_sens_config config = {0};


    config.type = BMI2_ACCEL;


    bmi2_get_sensor_config(
        &config,
        1,
        &bmi
    );


    config.cfg.acc.odr =
        BMI2_ACC_ODR_100HZ;


    config.cfg.acc.range =
        BMI2_ACC_RANGE_4G;


    config.cfg.acc.bwp =
        BMI2_ACC_NORMAL_AVG4;


    bmi2_set_sensor_config(
        &config,
        1,
        &bmi
    );



    config.type = BMI2_GYRO;


    config.cfg.gyr.odr =
        BMI2_GYR_ODR_100HZ;


    config.cfg.gyr.range =
        BMI2_GYR_RANGE_2000;


    bmi2_set_sensor_config(
        &config,
        1,
        &bmi
    );

    uint8_t sens_list[] = {BMI2_ACCEL, BMI2_GYRO};
    rslt = bmi2_sensor_enable(sens_list, 2, &bmi);
    if (rslt != BMI2_OK)
    {
        printf("BMI270 SENSOR ENABLE ERROR: %d\r\n", rslt);
    }

}



int main(void)
{

    HAL_Init();

    SystemClock_Config();


    MX_GPIO_Init();

    MX_I2C1_Init();

    MX_USART1_UART_Init();



    BMI270_Init();



    while(1)
    {
        struct bmi2_sensor_data sensor[2] = {0};
        int8_t rslt;

        sensor[0].type = BMI2_ACCEL;
        sensor[1].type = BMI2_GYRO;

        rslt = bmi2_get_sensor_data(sensor, 2, &bmi);
        if (rslt == BMI2_OK)
        {
            printf("ACC:%d %d %d\r\n",
                   sensor[0].sens_data.acc.x,
                   sensor[0].sens_data.acc.y,
                   sensor[0].sens_data.acc.z);
            printf("GYR:%d %d %d\r\n",
                   sensor[1].sens_data.gyr.x,
                   sensor[1].sens_data.gyr.y,
                   sensor[1].sens_data.gyr.z);
        }
        else
        {
            printf("BMI270 DATA ERROR: %d\r\n", rslt);
        }

        HAL_Delay(100);

    }

}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}
