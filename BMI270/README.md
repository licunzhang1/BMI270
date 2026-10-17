# BMI270 6-axis IMU driver -- STM32F103C8T6

Drives the Bosch BMI270 6-axis inertial measurement unit over hardware I2C1 on
an STM32F103C8T6 (Blue Pill) and streams raw acceleration and angular rate over
USART1.

Built on the official Bosch Sensortec driver, with UART telemetry.

---

## Hardware

| Item | Detail |
|---|---|
| MCU | STM32F103C8T6 (Blue Pill), Cortex-M3 @ 72 MHz |
| System clock | external 8 MHz crystal x PLL9 = 72 MHz |
| Sensor | Bosch BMI270 |
| Log output | USART1 @ 115200-8-N-1 |

> Note: the code forces HSE (`RCC_HSE_ON`). The board must have an 8 MHz crystal
> fitted, otherwise it spins forever inside `Error_Handler()` with no output at all.

### Wiring

| STM32F103 | BMI270 | Note |
|---|---|---|
| PB6 | SCL | I2C1 clock |
| PB7 | SDA | I2C1 data |
| 3V3 | VDD / VDDIO | supply |
| GND | GND | common ground |
| GND | SDO | tied low, giving I2C address **0x68** |
| VDDIO | CSB | pulled high selects I2C mode (low selects SPI) |

| STM32F103 | USB-TTL | Note |
|---|---|---|
| PA9 | RXD | USART1 TX |
| PA10 | TXD | USART1 RX |

- The I2C bus needs pull-ups. The common BMI270 breakout boards already carry
  4.7 kOhm pull-ups; a bare chip needs them fitted externally.
- If SDO is tied to `VDDIO` instead, the I2C address becomes `0x69`, and
  `BMI270_ADDR` in `Core/Src/bmi270_port.c` has to be changed to match.

## Sensor configuration

| Sensor | ODR | Range | Bandwidth |
|---|---|---|---|
| Accelerometer | 100 Hz | +/-4 g | Normal (avg 4) |
| Gyroscope | 100 Hz | +/-2000 dps | Normal |

The configuration lives in `BMI270_Init()` in `Core/Src/main.c`.

**Raw value scaling** (the output is in LSB, not physical units):

| Quantity | Sensitivity |
|---|---|
| Acceleration | +/-4 g -> **8192 LSB/g** |
| Angular rate | +/-2000 dps -> **16.4 LSB/dps** |

For example, `ACC: 0 0 8192` means roughly +1 g on the Z axis.

## Serial output

After power-up:

```
BMI270 OK
ACC:123 -45 8123
GYR:12 -8 105
ACC:118 -44 8102
GYR:11 -9 103
...
```

- `BMI270 OK` / `BMI270 ERROR` -- the result of `bmi270_init()`.
- `ACC:` / `GYR:` -- **raw int16 values** for the X, Y and Z axes, one group
  every 100 ms (10 Hz).
- On failure it also prints `BMI270 SENSOR ENABLE ERROR: <code>` or
  `BMI270 DATA ERROR: <code>`, where `<code>` is the negative error code from
  the Bosch driver (defined in `Core/Inc/bmi2_defs.h`).

## Build

Requires `cmake`, `ninja` and `arm-none-eabi-gcc` (both are bundled with
STM32CubeCLT and STM32CubeIDE).

```bash
cmake --preset Debug
cmake --build --preset Debug

# or Release
cmake --preset Release
cmake --build --preset Release
```

The artifact is `build/<preset>/BMI270.elf`.

```bash
# produce flashable bin / hex
arm-none-eabi-objcopy -O binary build/Debug/BMI270.elf build/Debug/BMI270.bin
arm-none-eabi-objcopy -O ihex   build/Debug/BMI270.elf build/Debug/BMI270.hex
```

- CMake presets: `CMakePresets.json`
- Toolchain definition: `cmake/gcc-arm-none-eabi.cmake`
- Debug configuration (J-Link GDB Server): `.vscode/launch.json`
- Project source of truth: `BMI270.ioc` (STM32CubeMX, can regenerate the tree)

## Layout

```
Core/Src/main.c            application logic: clocks, peripheral init, BMI270 setup, main loop
Core/Src/bmi270_port.c     * porting layer: I2C read/write + microsecond delay
Core/Src/bmi270.c          official Bosch BMI270 driver (unmodified)
Core/Src/bmi2.c            official Bosch common driver layer (unmodified)
Core/Inc/bmi270_port.h     porting layer interface
Core/Inc/bmi2_defs.h       register addresses and configuration constants
Drivers/                   STM32F1xx HAL + CMSIS
cmake/                     toolchain file and CubeMX-generated CMake fragments
BMI270.ioc                 STM32CubeMX project file
```

## Porting notes

`Core/Src/bmi270_port.c` implements the three callbacks the Bosch driver
requires:

| Callback | Implementation |
|---|---|
| `read` / `write` | `HAL_I2C_Mem_Read/Write`, 8-bit register address, 100 ms timeout |
| `delay_us` | DWT cycle counter (`DWT->CYCCNT`) |

**Why `delay_us` uses the DWT counter.** This started out as a busy `for` loop.
At `-Os` (the Release preset) GCC deletes a side-effect-free empty loop
entirely: the function was measured to compile down to a single `bx lr`, so every
delay in the driver became zero and `bmi270_init()` failed immediately. Both the
2 ms wait after the soft reset and the status polling after the configuration
upload depend on that delay. The DWT cycle counter is not removed by the
optimiser and does not depend on the compiler flags.

The delay is converted from `SystemCoreClock` at run time, so **changing the core
clock does not require touching this file**.

## Known limitations

- The serial output is **raw LSB**, not converted to g or dps (see the scaling
  factors above).
- Sampling is a 10 Hz polled read; neither the BMI270 FIFO nor the data-ready
  interrupt is used, so it is not suitable for high-rate capture.
- None of the Bosch built-in features are enabled (step counter, wrist wake-up,
  significant-motion / any-motion detection, and so on).
- I2C runs at 100 kHz. It could be raised to 400 kHz to speed up the 8 KB
  configuration file upload.
- `intf_ptr` is unused (the porting layer hard-codes the address), so passing
  `NULL` for it is safe.

## Third-party code

This repository contains the following third-party sources. Copyright belongs to
their respective owners; observe the corresponding licences.

| Component | Origin | Licence | Licence file |
|---|---|---|---|
| BMI270 / BMI2 driver | [Bosch Sensortec](https://github.com/boschsensortec/BMI270_SensorAPI) | BSD-3-Clause | copyright notice at the head of each source file |
| STM32F1xx HAL Driver | STMicroelectronics | BSD-3-Clause | `Drivers/STM32F1xx_HAL_Driver/LICENSE.txt` |
| CMSIS | ARM | Apache-2.0 | `Drivers/CMSIS/LICENSE.txt` |
| CMSIS Device STM32F1xx | STMicroelectronics | Apache-2.0 | `Drivers/CMSIS/Device/ST/STM32F1xx/LICENSE.txt` |

`Core/Src/bmi270.c`, `Core/Src/bmi2.c` and `Core/Inc/bmi2*.h` are the official
Bosch drivers and are **kept exactly as shipped**; the copyright notices and
licence terms in their file headers are preserved in full.

### What was trimmed

To keep the repository small, the following files, unrelated to this project
(Cortex-M3, HAL layer only), were removed. This **does not affect the build**: all
25 source files were checked with `-fsyntax-only` and produced zero errors and
zero warnings.

- In `Drivers/CMSIS/Include/`, every core header other than `core_cm3.h`,
  `cmsis_*.h` and `mpu_armv7.h` (M0 / M0+ / M1 / M4 / M7 / M23 / M33, ARMv8-M,
  SC000 / SC300).
- `Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_*.h` -- this project uses the HAL
  only, never the LL layer.

To get the complete set back, open `BMI270.ioc` in STM32CubeMX and regenerate.

## License

This repository does **not yet declare** a licence for the application code
(`Core/Src/main.c`, `Core/Src/bmi270_port.c`, `CMakeLists.txt` and so on). If you
want others to be free to use it, add a `LICENSE` file such as
[MIT](https://choosealicense.com/licenses/mit/) or BSD-3-Clause.
