# BMI270

BMI270 6-axis IMU driver for STM32F103C8T6 (Blue Pill) over hardware I2C1,
with UART telemetry of raw acceleration and angular rate.

Built on the official Bosch Sensortec BMI270 driver.

## Quick facts

| | |
|---|---|
| MCU | STM32F103C8T6, Cortex-M3 @ 72 MHz (external 8 MHz crystal required) |
| Sensor | Bosch BMI270, I2C address `0x68` |
| Bus | I2C1 on PB6 (SCL) / PB7 (SDA) |
| Output | USART1 @ 115200-8-N-1, raw int16 `ACC:` / `GYR:` lines at 10 Hz |
| Build | CMake presets, `arm-none-eabi-gcc` |

```bash
cmake --preset Debug
cmake --build --preset Debug
```

The firmware is a CubeMX project under [`BMI270/`](BMI270/). Wiring, sensor
configuration, raw-value scaling, porting notes, known limitations and the
third-party licence inventory are documented in
[`BMI270/README.md`](BMI270/README.md).

## Licence

The application code does not declare a licence yet. Vendored third-party
components (Bosch BMI270 driver, STM32F1xx HAL, CMSIS) keep their own licences;
see the inventory in [`BMI270/README.md`](BMI270/README.md).
