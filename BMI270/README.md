# BMI270 六轴 IMU 驱动 — STM32F103C8T6

在 STM32F103C8T6（Blue Pill）上通过硬件 I2C1 驱动 Bosch BMI270 六轴惯性测量单元，
经 USART1 输出加速度与角速度原始数据。

> 6-axis IMU (accelerometer + gyroscope) driver for STM32F103C8T6 over I2C1,
> built on the official Bosch Sensortec driver, with UART telemetry.

---

## 硬件 / Hardware

| 项目 | 说明 |
|---|---|
| MCU | STM32F103C8T6（Blue Pill），Cortex-M3 @ 72 MHz |
| 系统时钟 | 外部 8 MHz 晶振 × PLL9 = 72 MHz |
| 传感器 | Bosch BMI270 |
| 日志输出 | USART1 @ 115200-8-N-1 |

> ⚠️ 代码强制使用 HSE（`RCC_HSE_ON`）。板子必须焊有 8 MHz 晶振，否则会卡在
> `Error_Handler()` 的死循环里且没有任何输出。

### 接线 / Wiring

| STM32F103 | BMI270 | 说明 |
|---|---|---|
| PB6 | SCL | I2C1 时钟 |
| PB7 | SDA | I2C1 数据 |
| 3V3 | VDD / VDDIO | 供电 |
| GND | GND | 共地 |
| GND | SDO | 接地 → I2C 地址 **0x68** |
| VDDIO | CSB | 拉高选中 I2C 模式（拉低为 SPI）|

| STM32F103 | USB-TTL | 说明 |
|---|---|---|
| PA9 | RXD | USART1 TX |
| PA10 | TXD | USART1 RX |

- I2C 总线需要上拉电阻。常见的 BMI270 模块已板载 4.7 kΩ 上拉；裸芯片必须外接。
- 若把 SDO 接 `VDDIO`，I2C 地址变为 `0x69`，需同步修改
  `Core/Src/bmi270_port.c` 中的 `BMI270_ADDR`。

## 传感器配置 / Sensor configuration

| 传感器 | ODR | 量程 | 带宽 |
|---|---|---|---|
| 加速度计 | 100 Hz | ±4 g | Normal（avg 4）|
| 陀螺仪 | 100 Hz | ±2000 dps | Normal |

配置位于 `Core/Src/main.c` 的 `BMI270_Init()`。

**原始值换算**（输出的是 LSB，不是物理量）：

| 物理量 | 灵敏度 |
|---|---|
| 加速度 | ±4 g → **8192 LSB/g** |
| 角速度 | ±2000 dps → **16.4 LSB/dps** |

例如 `ACC: 0 0 8192` 表示 Z 轴约 +1 g。

## 串口输出 / Serial output

上电后：

```
BMI270 OK
ACC:123 -45 8123
GYR:12 -8 105
ACC:118 -44 8102
GYR:11 -9 103
...
```

- `BMI270 OK` / `BMI270 ERROR` —— `bmi270_init()` 的结果
- `ACC:` / `GYR:` —— X Y Z 三轴的**原始 int16 值**，每 100 ms 一组（10 Hz）
- 出错时还会打印 `BMI270 SENSOR ENABLE ERROR: <code>` 或 `BMI270 DATA ERROR: <code>`，
  `<code>` 为 Bosch 驱动的负错误码（定义见 `Core/Inc/bmi2_defs.h`）

## 编译 / Build

依赖 `cmake`、`ninja`、`arm-none-eabi-gcc`（STM32CubeCLT 或 STM32CubeIDE 自带）。

```bash
cmake --preset Debug
cmake --build --preset Debug

# 或 Release
cmake --preset Release
cmake --build --preset Release
```

产物位于 `build/<preset>/BMI270.elf`。

```bash
# 生成可烧写的 bin / hex
arm-none-eabi-objcopy -O binary build/Debug/BMI270.elf build/Debug/BMI270.bin
arm-none-eabi-objcopy -O ihex   build/Debug/BMI270.elf build/Debug/BMI270.hex
```

- CMake 预设：`CMakePresets.json`
- 工具链定义：`cmake/gcc-arm-none-eabi.cmake`
- 调试配置（J-Link GDB Server）：`.vscode/launch.json`
- 工程源头：`BMI270.ioc`（STM32CubeMX，可重新生成）

## 目录结构 / Layout

```
Core/Src/main.c            应用逻辑：时钟、外设初始化、BMI270 配置、主循环
Core/Src/bmi270_port.c     ★ 移植层：I2C 读写 + 微秒延时
Core/Src/bmi270.c          Bosch 官方 BMI270 驱动（未修改）
Core/Src/bmi2.c            Bosch 官方通用驱动层（未修改）
Core/Inc/bmi270_port.h     移植层接口
Core/Inc/bmi2_defs.h       寄存器地址与配置常量
Drivers/                   STM32F1xx HAL + CMSIS
cmake/                     工具链与 CubeMX 生成的 CMake 片段
BMI270.ioc                 STM32CubeMX 工程文件
```

## 移植层说明 / Porting notes

`Core/Src/bmi270_port.c` 实现 Bosch 驱动要求挂载的三个回调：

| 回调 | 实现 |
|---|---|
| `read` / `write` | `HAL_I2C_Mem_Read/Write`，8 位寄存器地址，100 ms 超时 |
| `delay_us` | DWT 周期计数器（`DWT->CYCCNT`）|

**为什么 `delay_us` 用 DWT**：最初这里是空转 `for` 循环。GCC 在 `-Os`（Release 预设）
下会把这种无副作用的空循环整个删掉——实测该函数被编译成单独一条 `bx lr`，
于是驱动的所有延时归零，`bmi270_init()` 随即失败（软复位后的 2 ms 等待、
配置加载后的状态轮询都依赖它）。DWT 周期计数器不会被优化掉，也不受编译选项影响。

需要延时的时间由 `SystemCoreClock` 动态换算，**更换主频无需改动此文件**。

## 已知限制 / Known limitations

- 串口输出的是**原始 LSB**，未换算成 g / dps（换算系数见上文）
- 采用 10 Hz 轮询读取，未使用 BMI270 的 FIFO 或数据就绪中断；不适合高速采样场景
- 未启用任何 Bosch 内置功能（计步、抬手唤醒、显著运动/任何运动检测等）
- I2C 运行在 100 kHz，可提高到 400 kHz 以加快 8 KB 配置文件的加载
- 未使用 `intf_ptr`（移植层中硬编码地址），因此该指针传 `NULL` 是安全的

## 第三方代码 / Third-party code

本仓库包含以下第三方源码，版权归各自所有者，使用时请遵守对应许可证：

| 组件 | 来源 | 许可证 | 许可证文件 |
|---|---|---|---|
| BMI270 / BMI2 驱动 | [Bosch Sensortec](https://github.com/boschsensortec/BMI270_SensorAPI) | BSD-3-Clause | 各源文件头部版权声明 |
| STM32F1xx HAL Driver | STMicroelectronics | BSD-3-Clause | `Drivers/STM32F1xx_HAL_Driver/LICENSE.txt` |
| CMSIS | ARM | Apache-2.0 | `Drivers/CMSIS/LICENSE.txt` |
| CMSIS Device STM32F1xx | STMicroelectronics | Apache-2.0 | `Drivers/CMSIS/Device/ST/STM32F1xx/LICENSE.txt` |

`Core/Src/bmi270.c`、`Core/Src/bmi2.c`、`Core/Inc/bmi2*.h` 为 Bosch 官方驱动，
**保持原样未作修改**，其版权声明与许可证条款完整保留在文件头部。

### 精简说明

为控制仓库体积，以下与本工程（Cortex-M3、且只用 HAL 层）无关的文件已删除，
**不影响编译**（已对全部 25 个源文件做 `-fsyntax-only` 检查，零错误零警告）：

- `Drivers/CMSIS/Include/` 中除 `core_cm3.h`、`cmsis_*.h`、`mpu_armv7.h` 之外的其它内核头文件
  （M0 / M0+ / M1 / M4 / M7 / M23 / M33、ARMv8-M、SC000 / SC300）
- `Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_*.h` —— 本工程只用 HAL，未使用 LL 层

如需完整文件，用 STM32CubeMX 打开 `BMI270.ioc` 重新生成即可。

## 许可证 / License

本仓库**尚未声明**应用代码（`Core/Src/main.c`、`Core/Src/bmi270_port.c`、
`CMakeLists.txt` 等）的许可证。如需他人可自由使用，建议补充一份
[MIT](https://choosealicense.com/licenses/mit/) 或 BSD-3-Clause 的 `LICENSE` 文件。
