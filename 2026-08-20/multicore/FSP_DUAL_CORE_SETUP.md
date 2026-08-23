# RA8P1 M85/M33 dual-core setup

The source code is ready for a two-project RA FSP Solution. The existing
`rainy` project remains the known-good single-core fallback and must not be
converted in place.

## 1. Create the project pair

1. In e2 studio select `File > New > Renesas RA FSP Solution`.
2. Select RA8P1, Flat/Bare Metal, CPU0 Cortex-M85 and CPU1 Cortex-M33.
3. Keep the board clock settings identical to the current `rainy` project.
4. Use these projects: `rainy_dual_CPU0` and `rainy_dual_CPU1`.
5. CPU0 must call `R_BSP_SecondaryCoreStart()`; this is already done by
   `platform_services_init()` when `PLATFORM_SERVICES_MULTICORE=1`.

## 2. Resource ownership

CPU0 / M85 owns CEU, OV5640 SCCB pins, Ethos-U55, SPI1/ILI9488, SDHI0,
DMAC0, GPT motor outputs and all actuator GPIO.

CPU1 / M33 owns IIC1/FT6336 and SCI4/weight UART:

| Service | Instance | Pins/settings |
|---|---|---|
| Touch | `g_i2c_master0` | IIC1, P511 SDA, P512 SCL, FT6336 RST P105 |
| Weight | `g_weight_uart` | SCI4, P715 RX, P714 TX, 9600 8N1 |

Do not configure IIC1 or SCI4 in CPU0 after dual-core migration. Do not
configure camera, display, SDHI, NPU or motor peripherals in CPU1.

## 3. IPC stack on both cores

Add `New Stack > System > IPC (r_ipc)` on both projects:

| Project | Name | Channel | Callback | IRQ |
|---|---|---:|---|---|
| CPU0 | `g_ipc_m85` | 0 | `platform_services_ipc_callback` | enabled, priority 5 |
| CPU1 | `g_ipc_m33` | 0 | `m33_services_ipc_callback` | enabled, priority 5 |

The channel number must match. One FSP 6.4 IPC instance maps both the local
send FIFO and receive FIFO; enabling its interrupt/callback allows the same
instance to receive while `messageSend()` uses its transmit FIFO.

## 4. Source placement

CPU0 uses the current application sources, including:

- `src/platform_services.c/.h`
- `src/multicore_protocol.h`
- camera, model, display, storage and machine-control sources

Define `PLATFORM_SERVICES_MULTICORE=1` in the CPU0 compiler preprocessor
settings. Exclude `ft6336.c` and `weight_sensor.c` from the CPU0 build after
their peripherals have moved to CPU1.

CPU1 uses:

- `multicore/m33/hal_entry.c`
- `multicore/m33/m33_services.c/.h`
- `src/multicore_protocol.h`
- `src/ft6336.c/.h`
- `src/weight_sensor.c/.h`

Add `src` and `multicore/m33` to CPU1 include paths.

## 5. Memory starting point

Use the FSP Solution memory editor. A conservative first allocation is:

- CPU0 Flash: 896 KB
- CPU1 Flash: 128 KB
- CPU0 internal RAM: 1536 KB
- CPU1 internal RAM: 336 KB
- External SDRAM: CPU0 only in the first integration stage

Generate both projects after changing the solution memory map. Never copy the
single-core linker script into either dual-core project.

## 6. Build, download and acceptance

1. Generate and build CPU0 first, then CPU1, or build the Solution project.
2. Confirm both `Debug/*.elf` files exist.
3. Use the CPU1 `Debug_Multicore Launch Group` to download both images.
4. The top-left indicator is split in dual-core mode: left is M33 online,
   right is FT6336 ready. Green/cyan is healthy; red is unavailable.
5. Touch a button and verify it responds while camera/NPU preview continues.
6. Feed weight frames and verify the dashboard value changes.
7. Pause CPU1 for more than three seconds: CPU0 must mark M33 offline without
   freezing camera, inference, display or motor safety logic.
8. Resume CPU1 and confirm the health packet restores online state.

The single-core fallback remains available by compiling CPU0 with
`PLATFORM_SERVICES_MULTICORE=0`.
