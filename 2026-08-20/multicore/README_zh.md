# RA8P1 Cortex-M85/M33 协同版落地说明

## 1. 当前协同架构

本次迁移采用“高吞吐视觉主核 + 低速外设服务核”的分工：

| 核心 | 负责功能 | 原因 |
|---|---|---|
| Cortex-M85 / CPU0 | OV5640、CEU 双缓冲、Ethos-U55 NPU、YOLO 后处理、ILI9488、SDHI、步进电机、舵机、分拣状态机 | 这些功能共享图像帧、推理结果或实时运动状态，放在同一核心避免跨核搬运大块图像 |
| Cortex-M33 / CPU1 | FT6336 触摸、称重串口、外设健康监测、M85 心跳监测 | 数据量小、周期明确，适合从视觉主循环中剥离 |

SD 卡暂时保留在 M85。拍照写卡直接使用刚完成采集的帧缓冲，如果迁到
M33，需要额外处理共享 SDRAM、Cache 一致性、跨核所有权和 SDHI DMA，风险和
复制开销都高于收益。当前方案已经实现了用户要求的 M33 协同，同时优先保证
摄像头、推理和拍照稳定。

两个核心只通过 32 位 IPC 消息传小数据，不通过 IPC 传图像指针。消息协议位于
`src/multicore_protocol.h`。

## 2. 已经完成的代码

- `src/platform_services.c/.h`：M85 的服务抽象层。单核模式直接读外设；双核
  模式从 IPC 快照读取触摸、重量和健康状态。
- `src/multicore_protocol.h`：触摸、重量、健康、心跳、重量诊断的 32 位协议。
- `multicore/m33/m33_services.c/.h`：M33 轮询触摸、接收称重 UART、发送健康包。
- `multicore/m33/hal_entry.c`：M33 裸机入口。
- `src/machine_control.c/.h`：称重值改由服务层注入，分拣状态机不再直接占用
  UART 驱动。
- `src/hal_entry.c`：M85 主循环不再直接访问触摸和称重；每 32 帧向 M33 发送
  心跳，M33 掉线不会阻塞摄像头、NPU、显示和电机安全状态。

`PLATFORM_SERVICES_MULTICORE=0` 是现有工程的安全回退模式；设置为 1 后才引用
FSP 生成的 IPC 实例并启动 M33。

## 3. 在 e2 studio 创建双核工程

不要把当前能正常烧录的 `rainy` 工程直接改成 CPU1。使用：

1. `File > New > Renesas RA FSP Solution`。
2. 选择当前 RA8P1 的同一具体料号、Flat/Bare Metal、CPU0 + CPU1。
3. 建议命名为 `rainy_dual`，生成 `rainy_dual_CPU0` 和
   `rainy_dual_CPU1` 两个子工程。
4. Solution 的 Clock 设置与当前 `rainy` 保持一致。
5. 在 Solution 的 Memories 页面先分配：CPU0 Flash 896 KB、CPU1 Flash
   128 KB、CPU0 SRAM 1536 KB、CPU1 SRAM 336 KB。外部 SDRAM第一阶段归 CPU0。
6. 不要复制当前单核工程的 linker script；两个核心的链接地址必须由 Solution
   统一生成。

## 4. CPU0 / M85 FSP 配置

CPU0 保留当前工程中的摄像头、CEU、NPU、SPI 显示、SDHI、DMAC、GPT 和执行器
GPIO 配置。添加：

| Stack | Name | Channel | Callback | IRQ |
|---|---|---:|---|---|
| System > IPC (`r_ipc`) | `g_ipc_m85` | 0 | `platform_services_ipc_callback` | Priority 5 |

FSP 6.4 的一个 IPC 实例同时持有本核 TX 和 RX FIFO 映射，因此每核只添加一个
实例。接收消息必须启用 IRQ 和 callback。

CPU0 编译器预处理宏增加：

```text
PLATFORM_SERVICES_MULTICORE=1
```

CPU0 添加当前 `src` 下的业务源文件，并确保包含
`platform_services.c` 和 `multicore_protocol.h`。正式双核版中，IIC1 和 SCI4
归 CPU1，CPU0 不再配置这两个外设；`ft6336.c`、`weight_sensor.c` 也不加入
CPU0 编译。

## 5. CPU1 / M33 FSP 配置

CPU1 添加以下外设：

| 功能 | FSP 名称 | 配置 |
|---|---|---|
| IPC | `g_ipc_m33` | Channel 0，callback=`m33_services_ipc_callback`，IRQ Priority 5 |
| 触摸 I2C | `g_i2c_master0` | IIC1，SDA=P511，SCL=P512，callback=`ft6336_i2c_callback` |
| 触摸复位 | GPIO | P105，Output High |
| 称重串口 | `g_weight_uart` | SCI4，RX=P715，TX=P714，9600、8N1，callback=`weight_uart_callback` |

CPU1 工程只加入：

```text
multicore/m33/hal_entry.c
multicore/m33/m33_services.c
src/multicore_protocol.h
src/ft6336.c
src/ft6336.h
src/weight_sensor.c
src/weight_sensor.h
```

Include Paths 增加当前工程的 `src` 和 `multicore/m33`。不要把摄像头、显示、
SDHI、NPU 或电机模块加入 CPU1，也不要让两个核心同时配置同一个引脚或外设。

## 6. 生成、编译与烧录

1. 在 Solution 中先 `Generate Project Content`。
2. 分别确认 CPU0 和 CPU1 均能无错误 Build，并各自产生一个 `.elf`。
3. CPU0 的 `platform_services_init()` 会调用 `R_BSP_SecondaryCoreStart()`。
4. 使用 Solution 自动生成的 Multicore Launch Group，同时下载两个 ELF；只烧
   CPU0 不会得到 M33 外设服务。
5. 如果 Launch Group 连接 CPU1 失败，先确认芯片保护级别和 TrustZone 边界，
   再重新初始化器件。

## 7. 屏幕状态和验收

双核模式下，屏幕左上角 16 x 16 状态块分成两半：

| 显示 | 含义 |
|---|---|
| 左半绿色 | M33 在线，且 M33 最近 3 秒内收到过 M85 心跳 |
| 左半黄色 | 收到 M33 健康包，但 M33 尚未确认 M85 心跳 |
| 左半红色 | 3 秒内未收到 M33 健康包或 IPC 未启动 |
| 右半青色 | FT6336 初始化成功 |
| 右半红色 | 触摸总线、应答或芯片 ID 异常 |

验收顺序：

1. 上电后左半由黄转绿、右半为青色。
2. 开始/暂停/结束和拍照模式触摸正常，摄像头画面不再因 I2C 轮询变花。
3. 称重数据变化时仪表盘同步更新。
4. 暂停 CPU1 超过 3 秒：左半变红，但摄像头、显示和推理继续运行。
5. 恢复 CPU1：健康包到达后状态自动恢复。
6. CPU0 单独调试时可把宏改回 0，确认现有单核回退版仍可工作。

## 8. IPC 消息频率

- 触摸：10 ms 采样；状态变化立即发送，静止时每 100 ms 刷新一次。
- 称重：每收到一个有效 UART 帧发送重量和诊断消息。
- M33 健康包：每 1 秒发送一次。
- M85 心跳：每 32 个摄像头帧发送一次。
- 双方超时：3 秒。

发送采用非阻塞 FIFO。FIFO 满时跳过本次低速状态包并累计 drop 计数，不允许
触摸或称重反向卡住视觉主循环。
