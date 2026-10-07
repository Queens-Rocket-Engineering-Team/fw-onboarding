# Firmware onboarding with ThreadX

This project is an Eclipse ThreadX foundation for the **STM32F103CBT6**. It builds
C firmware with **GNU Tools for STM32, CMake, and Ninja**, and supports editing,
flashing, and debugging in **Visual Studio Code**. VS Code is the editor; the Arm
compiler produces the firmware that runs on the microcontroller.

Two threads increment debugger-visible counters. Sensor drivers and altitude
calculations are future exercises. This example needs no LED, UART, Arduino core,
or `.ino` sketch.

## Target and application

| Setting | Value |
| --- | --- |
| MCU | STM32F103CBT6, Arm Cortex-M3 |
| Flash / RAM | 128 KiB / 20 KiB |
| Core clock | 8 MHz internal HSI, no PLL |
| ThreadX tick | SysTick, 1,000 ticks per second |
| HAL millisecond tick | TIM2, IRQ priority 14, separate from ThreadX |
| Debug interface | SWD through an ST-LINK probe |

TIM2 must use interrupt priority **14**, above PendSV's **15**. This ThreadX port
idles inside PendSV, so TIM2 must be able to preempt it to keep HAL timeouts
advancing while all threads sleep. Interrupt priorities are separate from the
thread priorities below.

[`ThreadX/app_threadx.c`](ThreadX/app_threadx.c) defines the application:

| Thread | Counter to watch | Sleep between increments | Priority | Stack |
| --- | --- | --- | --- | --- |
| Heartbeat 250 ms | `heartbeat_250ms_count` | 250 ticks | 10 | 1 KiB |
| Heartbeat 1 s | `heartbeat_1s_count` | 1,000 ticks | 20 | 1 KiB |

Lower ThreadX priority numbers run first. Both threads use statically allocated
stacks and `tx_thread_sleep()` so that other work can run. These are relative
delays, not precise periodic deadlines. [`ThreadX/tx_user.h`](ThreadX/tx_user.h)
configures the kernel; Debug builds enable stack checking. The timer thread has
its own 1 KiB stack.

## Install the tools

If this checkout already contains `.tools/cube/bin/cube.exe`, use the
[workspace-local setup](#reuse-workspace-local-tools-on-windows) below to reuse
its downloaded tools. For a fresh clone, follow these installation steps.

1. Install [Visual Studio Code](https://code.visualstudio.com/) and the
   [STM32CubeIDE for Visual Studio Code extension pack](https://marketplace.visualstudio.com/items?itemName=stmicroelectronics.stm32-vscode-extension)
   from **STMicroelectronics**. Accept this repository's extension recommendation
   when prompted. The pack includes CMake integration, clangd, ST-LINK debugging,
   and ThreadX inspection tools.
2. Open this repository's root folder. Accept the extension's prompts to install
   project dependencies, or use the terminal command in the next step. The
   project's bundle files select versions of GNU Tools for STM32, CMake, Ninja,
   and the debugger tools. Use **STM32Cube Bundle Manager** to inspect installed
   versions, and allow the extension to download device support as needed.
   Current ST releases use bundles; a separate STM32CubeCLT is unnecessary.
3. Open a new terminal and check `cube --version`, then install the project's
   tool requirements with `cube bundle install --project`. If `cube` is not found,
   restart VS Code after tool installation and consult
   [ST's installation guide](https://dev.st.com/stm32cube-docs/stm32cubecli/latest/en/docs/markup/STM32CubeCLI_Getting_Started/STM32CubeCLI_Installation.html).
4. On Windows, install **STM32Cube Resources → ST-Link USB Drivers** before
   connecting to a board. See [ST's driver instructions](https://dev.st.com/stm32cube-docs/stm32cubeide-vscode/latest/en/docs/markup/getting_started/installation.html).
5. Install standalone [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html)
   if you will edit MCU, clock, or peripheral settings. The committed source can
   be built without regenerating it.

Tool versions are recorded in the project's
[bundle requirements](.settings/bundles.store.json) and
[bundle lock file](.settings/bundles-lock.store.json). Keep both files under
version control so teammates install the same tool versions.

Create a working branch for your exercises:

```sh
git switch -c your-first-last
```

### Reuse workspace-local tools on Windows

For a checkout with tools installed under `.tools/`, open PowerShell outside
VS Code, change to the repository root, and run:

```powershell
$onboardingRoot = (Get-Location).Path
$env:CUBE_BUNDLE_PATH = Join-Path $onboardingRoot '.tools\bundles'
$env:CUBE_CACHE_PATH = Join-Path $onboardingRoot '.tools\cube-cache'
$env:CUBE_LOG_PATH = Join-Path $onboardingRoot '.tools\cube-logs'
$env:CMSIS_PACK_ROOT = Join-Path $onboardingRoot '.tools\packs'
$env:PATH = (Join-Path $onboardingRoot '.tools\cube\bin') + [IO.Path]::PathSeparator + $env:PATH
cube --version
```

The build commands below now use this checkout's installed bundles. To let the
ST extension use the same tools, close all VS Code windows and run `code .` from
this PowerShell session. An already running VS Code process can retain its old
environment, so opening another window is insufficient. These settings apply to
this shell and processes started from it; they do not change the global PATH.
`.tools/` is ignored by Git. A fresh clone installs the locked tools normally
through the extension or `cube bundle install --project`.

## Build

Select **CMake: Select Configure Preset → Debug** in VS Code. If the STM32 view has
not discovered the project, choose **Discover STM32Cube project**, select the
STM32F103CB device and GNU/GCC toolchain, and save. Then use **CMake: Configure**
and **CMake: Build**. Choose **Release** for the optimized configuration. Initial
automatic configuration is disabled so you can install the tools first. See
[ST's discovery guide](https://dev.st.com/stm32cube-docs/stm32cubeide-vscode/latest/en/docs/markup/getting_started/first_project_creation.html).

Equivalent terminal commands, run from the repository root:

```sh
cube bundle install --project
cube cmake --preset Debug
cube cmake --build --preset Debug
cube cmake --preset Release
cube cmake --build --preset Release
```

Outputs go into `build/Debug/` and `build/Release/`. Use `fw-onboarding.elf` for
debugging and `fw-onboarding.map` to inspect flash and RAM use. Account for all
thread stacks and the interrupt stack within the 20 KiB RAM limit. The generated
`STM32F103xx_FLASH.ld` reserves 1 KiB for the interrupt/main stack and 512 bytes
for the C heap, separately from the ThreadX stacks. Compilation does not verify
scheduler timing or board connectivity; run the checks below.

The `EWARM/` directory records the original IAR project. It is historical and is
not maintained as a ThreadX build. Use the GNU/CMake configuration.

## Flash, debug, and check the scheduler

1. Connect ST-LINK to the board's **SWDIO, SWCLK, ground, target voltage reference,
   and NRST**, using the board schematic. The launch configuration uses **Connect
   under reset**, so NRST must be connected. Power the board as required by its
   hardware design.
2. Build **Debug**, select **ThreadX: ST-LINK** in Run and Debug, and press **F5**.
   This builds the active CMake configuration, programs the MCU, and stops at
   `main()`. The ST extension resolves tool paths and the firmware image.
3. Continue for several seconds, then pause. Watch `heartbeat_250ms_count` and
   `heartbeat_1s_count`. Both should advance between runs, with about four fast
   increments per slow increment. They increment immediately on thread entry,
   so compare changes between observations.
4. Open **STM32CUBE RTOS** after the scheduler starts. Confirm **Heartbeat 250 ms**
   and **Heartbeat 1 s** exist with priorities 10 and 20 and mostly sleep. The
   launch profile selects ThreadX's `cortex_m3` debugger port.
5. Watch `uwTick` and `_tx_timer_system_clock`: both should advance at roughly
   1,000 counts per second while running freely. Debugger pauses can affect the
   clocks differently, so do not include paused time in timing measurements.
6. Set breakpoints on `Error_Handler`, `HardFault_Handler`, and, in Debug,
   `app_threadx_stack_error`. `app_threadx_last_error` should stay `TX_SUCCESS`
   (zero); `app_threadx_stack_error_thread` should stay null. Test repeated resets
   and a longer run before adding application work.

An empty RTOS view at `main()` is expected: no threads exist yet. If connection
fails, check the driver, target power, SWD/NRST wiring, and ST-LINK firmware in
**STM32CUBE DEVICES AND BOARDS**. Keep **Serial Wire** enabled; the original
`No_Debug` setting disabled SWD during initialization.

See [ST's debugger guide](https://dev.st.com/stm32cube-docs/stm32cubeide-vscode/latest/en/docs/markup/development/debug.html)
for the launch and RTOS inspection workflow.

### Verified baseline: 2026-10-07

Debug and Release builds and CubeMX regeneration checks passed. Both images were
flashed, verified, and checked while running through the ST tools. The Debug
image was also checked with GDB.

| Configuration | Flash used | RAM allocation, including configured reserves |
| --- | --- | --- |
| Debug | 13,192 bytes | 5,688 bytes |
| Release | 6,508 bytes | 5,688 bytes |

Over a 5.23-second Debug run, both HAL and ThreadX advanced 5,231 ticks; the
heartbeat counters advanced 21 and 5 times, with no recorded errors. Release
showed the same counter progression and approximately matching timebases. GDB
confirmed reset clears the counters, both threads have the expected priorities,
and each stack pointer is within its own 1 KiB stack. Debug was restored to the
board after testing.

The VS Code launch configuration was checked against ST's installed debugger
schema and ThreadX driver registry. The graphical RTOS view was not manually
tested; the hardware checks used command-line tools and GDB.

## Edit firmware and regenerate CubeMX code

Add application C source under `ThreadX/` and register it in the user-owned
top-level CMake build. Place changes to generated initialization in `USER CODE`
sections where available. Use ThreadX sleep/synchronization for thread waits;
`HAL_Delay()` busy-waits and should not implement thread periods. Reserve TIM2
for HAL and SysTick/PendSV for ThreadX.

The vendored kernel lives in `ThirdParty/ThreadX/`; the application and board
integration live in `ThreadX/`. Keep them outside CubeMX's generated source and
build areas (`Core/`, `Drivers/`, and `cmake/stm32cubemx/`). In particular, do not
move the kernel into `Middlewares/`: CubeMX can delete unregistered middleware
there during regeneration, even when user-code preservation is enabled.

Open `fw-onboarding.ioc` in STM32CubeMX to change hardware settings, retaining:

- **STM32F103CBTx** and the **8 MHz HSI** clock for this foundation.
- **Project Manager → Toolchain/IDE: CMake**, compiler **GCC**.
- **System Core → SYS → Debug: Serial Wire**, time base **TIM2**.
- **TIM2 interrupt priority 14**, above the kernel's **PendSV priority 15**.
- Kernel ownership of **PendSV** and **SysTick**, without competing generated C
  handlers for those exceptions.

Generate code and inspect the diff. Preserve `tx_kernel_enter()` after HAL/clock
initialization, the integration in `ThreadX/tx_initialize_low_level.S`, and the
ThreadX build sources/includes/definitions. Keep one HAL time-base implementation
and one TIM2 handler. Do not overwrite the vendored kernel with CubeMX middleware
or compile the upstream example startup files. Rebuild Debug and Release, review
the map, and repeat the hardware checks. ST describes generated versus user-owned
build files in its [CMake guide](https://dev.st.com/stm32cube-docs/stm32cubeide-vscode/latest/en/docs/markup/basic_concepts/cmake.html).

## Next exercise: an altimeter

Continue with the [altimeter exercises](docs/altimeter-exercises.md) after the
kernel works on your board. Sensor acquisition, altitude calculation, and flight
state detection are future work, not features of the current firmware.
