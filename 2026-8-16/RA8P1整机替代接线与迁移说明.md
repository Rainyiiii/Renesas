# RA8P1 逐飞板上插式整机迁移说明

> 本项目板是插在逐飞 CPKHMI-RA8P1 扩展板 P4~P10 上方的二级设备板，不替换逐飞板。逐飞侧完整对插关系见 `逐飞板P4-P10完整对插表.csv`，一设备一端子的总清单见 `RA8P1二级板设备端子清单.csv`；设备逐针接口、旧线束兼容定义、电源分区和 BOM 见 `RA8P1扩展板原理图设计规格.md`、`RA8P1扩展板接口表.csv` 与 `RA8P1扩展板BOM.csv`。

## 1. 目标与当前实现

本工程现在由一颗 RA8P1 完成 OV5640 采集、NPU 垃圾识别、ILI9488 显示、FT6336 触摸、传送/底盘步进、升降步进、三路舵机和称重串口，不再需要 K230 或 STM32F4。

上电默认处于 `STOP`，电机驱动保持禁用；摄像头预览仍刷新。触摸 `START` 后启动传送和识别分拣，`PAUSE` 停止运动但保留舵机位置，`STOP` 停止全部步进并让分拣舵机回安全位。

## 2. 扩展板引脚表

下表的 P4~P10 编号对应逐飞 CPKHMI-RA8P1 扩展板上的公针。本项目板底面焊对应通孔母座后直接向下插入；P5/P8 也必须焊接并走线，所有连接器都要求 1 脚对 1 脚且禁止镜像。

| 功能 | RA8P1 | 扩展板针脚 | FSP/电气模式 | 外部连接 |
|---|---|---:|---|---|
| 左侧 360° 舵机 PWM | P915 | P7-19 | GPT5 GTIOC5A, 50 Hz | 原 `ZhuaZi`/左分拣舵机 SIG |
| 右侧 360° 舵机 PWM | P914 | P7-2 | GPT5 GTIOC5B, 50 Hz | 原 `YunTai`/右分拣舵机 SIG |
| 270° 转盘舵机 PWM | P807 | P7-18 | GPT13 GTIOC13A, 50 Hz | 原 `ZhuanPan` SIG，保留接口 |
| 电机 1 STEP | P304 | P7-6 | GPT7 GTIOC7A | 独立输出 |
| 电机 2 STEP | P501 | P4-7 | GPT12 GTIOC12A | 独立比较输出 |
| 电机 3 STEP | P502 | P4-9 | GPT12 GTIOC12B | 独立比较输出 |
| 电机 4 STEP | P810 | P4-6 | GPT10 GTIOC10A | 独立输出，与 P811 舵机 4 存在定时器资源冲突 |
| 驱动方向 1 | P600 | P7-4 | GPIO Output Low | Driver 1 DIR |
| 驱动方向 2 | P106 | P7-8 | GPIO Output Low | Driver 2 DIR |
| 驱动方向 3 | PB07 | P7-10 | GPIO Output Low | Driver 3 DIR |
| 驱动方向 4 | P711 | P7-12 | GPIO Output Low | Driver 4 DIR |
| 四轮/传送使能 | P911 | P7-17 | GPIO Output High, 低有效 | 4 个驱动器 EN 并联 |
| 丝杆 STEP | P712 | P7-9 | GPT2 GTIOC2B, 默认 500 Hz | Lift Driver STEP |
| 丝杆 DIR | P903 | P7-14 | GPIO Output Low | Lift Driver DIR |
| 丝杆 EN | P904 | P7-16 | GPIO Output High, 低有效 | Lift Driver EN |
| 急停反馈 | P403 | P7-11 | GPIO Input Pull-up, 低有效 | 急停常开辅助触点，按下接地 |
| 丝杆上限位 | P910 | P4-14 | GPIO Input Pull-up, 低有效 | 上限位常开触点，触发接地 |
| 丝杆原点 | P913 | P4-16 | GPIO Input Pull-up, 低有效 | 原点常开触点，触发接地 |
| 称重串口 TX | P714 | P6-6 | SCI4 TXD4, 9600 8N1 | 接称重模块 RX |
| 称重串口 RX | P715 | P6-4 | SCI4 RXD4, 9600 8N1 | 接称重模块 TX |

屏幕使用一个独立 14Pin `J_DISPLAY`，线序固定为 `INT, NC, SDA, RST, SCL, MISO, LED, SCK, MOSI, DC, RESET, CS, GND, VCC`。实际连接为触摸 `P511/P105/P512`，LCD `P102/P708/P410/P409/P404`；背光默认跳帽接 3.3 V 常亮，也可切到 P415 控制。INT 和 MISO 当前不使用。

摄像头使用一个独立 2x9 `J_CAMERA`，完全照 ATK-MC5640 V1.2 原厂脚位：`1 GND, 2 3V3, 3 SCL/P602, 4 VSYNC/PB02, 5 SDA/P603, 6 HREF/PB03, 7 D0/P400, 8 RESET/P000, 9 D2/P405, 10 D1/P401, 11 D4/P700, 12 D3/P406, 13 D6/P702, 14 D5/P701, 15 PCLK/P414, 16 D7/P703, 17 PWDN/P001, 18 NC`。模块自带 24 MHz 晶振，不需要外接 XCLK。

上述屏幕、触摸、摄像头网络和 `P208-P211` 调试口必须保留且不能复用。屏幕与摄像头只能连接各自的唯一插座，不能再并接到冗余接口。

## 3. 扩展板电路要求

1. RA8P1 所有 GPIO 都是 3.3 V，不允许 5 V、12 V 或 24 V 信号直接进入。
2. 步进驱动器若不能可靠识别 3.3 V，使用 5 V 供电的 `74AHCT125/74HCT125` 缓冲 STEP、DIR、EN。每个输入加 10 kΩ 下拉或上拉，确保 RA8P1 复位期间驱动器关闭。
3. 舵机使用独立 5-6 V 大电流降压电源，3.3 V PWM 从 RA8P1 直接连接；舵机电源地与 RA8P1 地单点共地。不要从 RA8P1 的 3.3 V 引脚给舵机供电。
4. 12/24 V 光电传感器使用光耦或晶体管转换，光耦集电极接 RA 输入并上拉到 3.3 V，发射极接地，形成低有效信号。
5. 称重模块 TX 若是 5 V TTL，必须经电阻分压或电平转换后进入 P715；RA 的 P714 可直接驱动多数 TTL RX，不能接 RS-232 电平。
6. 急停除了接 P403 给软件检测，还必须硬件切断电机驱动 EN 或动力电源。软件急停不能替代硬件急停。
7. 舵机电源入口放置至少 470 uF 电解电容，每个接口附近放 100 nF；电机电源、舵机电源和 RA 供电采用星形回地。
8. 摄像头 D0~D7、PCLK、VSYNC、HREF 每根串联 22~33 Ω，电阻靠近摄像头座；PCLK 最短，并远离 STEP/PWM 和舵机、电机电源铜皮。
9. 推荐 4 层板并保留完整地平面；J_CAMERA 靠近 P5/P6/P8，J_DISPLAY 靠近 P6/P7/P8。

## 4. 分拣映射与状态显示

| YOLO 类别 | 屏幕名称 | 按原 STM32F4 程序迁移的动作 |
|---:|---|---|
| 0 | HARM | 左 360° 舵机正转 0.9 圈，再反转复位 |
| 1 | KITCHEN | 传送多走一段，右 360° 舵机正转 0.9 圈，再反转复位 |
| 2 | OTHER | 传送多走一段，右 360° 舵机反转 0.9 圈，再正转复位 |
| 3 | RECOVER | 左 360° 舵机反转 0.9 圈，再正转复位 |

摄像头画面底部状态格式为：`RUN/PAUSE/STOP Sx Cx Wxxx Hx Kx Ox Rx`。`S` 是分拣状态编号，`C` 是当前类别，`W` 是称重值，`H/K/O/R` 是四类累计数量。检测框仍显示类别和置信度。

识别结果连续 3 次一致且置信度不低于 0.45 才触发，避免单帧误识别。机械动作使用 DWT 硬件周期非阻塞计时，舵机动作期间摄像头、触摸和 NPU 仍继续运行。

## 5. 现场调参位置

机械参数集中在 `src/machine_control.c` 顶部：

- `FAR_CLASS_TRAVEL_MS`：KITCHEN/OTHER 比近端分类多走的时间。
- `SERVO_TURN_MS`：360° 舵机单向动作时间，当前沿用原程序的 1350 ms。
- `CONTINUOUS_STOP_ANGLE`：连续舵机停止脉宽对应值；舵机有缓慢自转时微调此值。
- `MOTOR_DEFAULT_HZ`：传送/底盘步进频率。
- `LIFT_DEFAULT_HZ`：丝杆步进频率。
- `DETECTION_MIN_SCORE`、`DETECTION_STABLE_TICKS`：触发阈值和稳定帧数。

## 6. e2 studio 使用

工程使用 FSP 6.4.0、LLVM for Arm 21.1.1。直接在 e2 studio 中选择 `Debug`，执行 Clean Project 后 Build Project，再用现有 J-Link 启动配置下载 `Debug/rainy.elf`。`configuration.xml` 已同步 SCI4 和所有新增 PWM/GPIO 引脚；若重新 Generate Project Content，生成后再完整 Build 一次即可。

当前构建占用：Flash 文本约 419066 B，静态 RAM 约 978220 B；RA8P1 配置为 1 MB Code Flash 和 1916928 B RAM，没有溢出。
