# RA8P1 垃圾分类控制迁移说明

## 已迁移设备

| 设备 | RA8P1 引脚 | FSP 功能 |
| --- | --- | --- |
| 主步进 DIR | P600 | GPIO Output Low |
| 主步进 STEP | P304 | GPT7 GTIOCA |
| 备用步进 DIR | P106 | GPIO Output Low |
| 备用步进 STEP | P501 | GPT12 GTIOCA |
| 左连续舵机 | P915 | GPT5 GTIOCA |
| 右连续舵机 | P914 | GPT5 GTIOCB |
| 备用连续舵机 | P807 | GPT13 GTIOCA |
| 称重模块 TX | P714 | SCI4 TXD4 |
| 称重模块 RX | P715 | SCI4 RXD4 |

摄像头、ILI9488 屏幕和 FT6336 触摸继续使用当前已经验证过的接口。旧 F4 工程中的四轮控制、升降电机、光控、限位和急停输入没有迁移。

## 当前自动分拣顺序（2026-07 按新机械布局重写）

实际布局：摄像头位于传送带中间，J_SERVO1 在摄像头前方（下游）、J_SERVO2 在摄像头后方（上游），每个舵机左右各一格共四格。

1. 上电后默认为 STOP，所有步进停止，舵机输出停转脉宽。
2. 触摸 RUN 后，主步进以 900 Hz 正向运行（向 J_SERVO1 方向送料）。
3. 同一类别连续识别 3 次且置信度不低于 0.45，进入分拣动作。
4. 前舵机的格子（默认 HARM/RECOVER）：传送带**继续正转** `TRAVEL_FRONT_MS` 到位。
5. 后舵机的格子（默认 KITCHEN/OTHER）：传送带**倒转** `TRAVEL_REAR_MS` 送回摄像头后方。
6. 到位停带；360° 舵机定速旋转 `SERVO_PUSH_MS` 推料，再**反向旋转** `SERVO_RETURN_MS` 显式回位（连续舵机只发停转脉宽不会自己回来），最后输出停转脉宽。
7. 传送带恢复正转，目标离开画面 4 帧后允许下一次识别。

类别与料斗位置不一致时，只需改 `src/machine_control.c` 顶部的 `g_sort_routes[]` 路由表（rear = 前/后舵机，push_left = 左/右格）。方向约定：J_SERVO1 加大角度值 = 向右推，J_SERVO2 加大角度值 = 向左推。详细标定流程见《工程对接文档.md》第 4 节。

## 称重协议

- 串口：SCI4，9600，8N1。
- 接收帧：`FF DATA_H DATA_L FE`。
- 数据按有符号 16 位解析，并保留旧 F4 程序的 4 倍标定。
- 未收到合法帧时屏幕显示 `W--`，收到后显示 `W数值`。

## 现有屏幕界面

- 顶部仪表区：运行模式、分拣阶段、重量、识别类别、置信度、目标中心 X/Y、四类完成数量、当前类别和总数。
- 中间画面区：OV5640 实时画面、YOLO 识别框和类别标签。
- 底部控制区：RUN、PAUSE、STOP 三个触摸按钮。
- 左上角触摸状态：绿色表示 FT6336 正常，红色表示触摸通信失败。

旧 STM32F4 屏幕中的 Scene 1/2 用于通知 K230 切换识别算法，当前 RA8P1 已直接运行四分类 NPU 模型，因此不再需要该选择。旧 Mode 2 依赖已删除的底盘和升降机构，也不再显示。

## e2 studio

`configuration.xml` 和 `ra_gen/pin_data.c` 已包含最终引脚配置。打开工程后可以直接 Build；如果点击 **Generate Project Content**，重点确认 P501 仍为 `GPT12 GTIOCA`，不要被恢复成普通 GPIO。

当前定时器由 `machine_control.c` 直接使用 FSP GPT API 打开，不需要在 Stacks 页面重复添加 GPT 实例。

## 当前编译容量

- Code Flash：约 421.1 KB / 1024 KB，剩余约 627.5 KB。
- RAM：约 978.2 KB / 1916.9 KB，剩余约 938.7 KB。
- 当前模型、双摄像头帧缓存和控制程序均已成功链接，没有 Flash 或 RAM 溢出。

## PCB 回来后的首次测试

1. 暂时不接舵机和步进驱动，只给 RA8P1、摄像头和屏幕供电，确认画面、识别框和触摸按钮正常。
2. 测量 P915、P914、P807：STOP 状态应为约 20 ms 周期、1.5 ms 高电平。
3. 只接主步进驱动的信号和公共 GND。按 RUN 后 P304 应有约 900 Hz 方波，P600 应为低电平。
4. 单独接两个舵机，确认 STOP 时不持续转动；若会慢转，微调对应的 `SERVO1_NEUTRAL_ANGLE` / `SERVO2_NEUTRAL_ANGLE`（这是回位精度的根基，必须先调到完全停住）。
5. 接称重模块，屏幕从 `W--` 变为重量数值后再做砝码标定。
6. 最后接外部 5 V、8.4 V、12 V 动力电源。RA8P1 GND 与动力板 GND 必须共地，但动力电源不要接入 RA8P1 的 3.3 V。

备用步进可用 `machine_stepper_select(MACHINE_STEPPER_BACKUP)` 切换；停机状态下可用 `machine_servo_route_to_backup(MACHINE_SERVO_LEFT)` 或 `MACHINE_SERVO_RIGHT` 将对应舵机逻辑改接到 P807。
