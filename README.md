# Firmware Onboarding
Firmware Onboarding via altimeter rewrite in Real Time Operating System format.

The goal of this onboarding is to serve as an intro to how embedded firmware works and how to use an RTOS (Real-Time Operating System), which is what we plan to use for the code that flies on the rocket. 

You will be writing firmware for our old Altimeter module, STINGER, to calculate the rocket's altitude live during flight.

## What's Firmware? What's an RTOS?
Firmware is the program that runs on a small chip (microcontroller/MCU) to perform  core functions and talk to the other peripherals on the board. This chip can only do one thing at a time, but an altimeter has several jobs: read the pressure sensor, read the motion sensor, blink a status light, communicate with other boards, fire parachutes.

There are several approaches for writing firmware, a simpler approach that we used on our old boards is to have a giant loop ("superloop") that runs all of these different tasks in a defined order. It can work, but a big issue is that everything always has to run in that order, and if a less important action (like blinking the status light) is slow, it can stop a more important task (like firing the parachutes) from running until much later. 

An **RTOS** solves this by juggling between these different tasks. In your program, each task is given its own small program called a **thread**. The RTOS switches threads automatically. Whenever a thread is waiting for something to happen, it gives a turn to another thread that needs the processing time. The thread with the highest priority wins. "Real-time" means can promise that urgent takss happen on time, which especially matters on a rocket. The RTOS we are using is called **ThreadX**.

## What's an Altimeter?

An altimeter is one of the core avionics functions any rocket needs. It is critical for recovery. The altimeter's main tasks are:
* Measure the rocket's altitude above ground level during flight
* Deploy parachutes: drogue at apogee (max altitude), main at a specific altitude.
* Communicate this data to other boards.

Deployment especially can get complicated, because it needs to figure out when the rocket reaches apogee and estimate state using the accelerometer and gyroscope, which gets more complicated above the speed of sound.

## Template Contents

This template already works, and you can flash it to the altimeter board to try it out. What it has:
* ThreadX (our RTOS) installed and starting up
* An LED blinking
* A thread reading the IMU (accelerometer + gyroscope) and printing it to your computer over serial

Drivers for the pressure sensor and IMU are already written for you in `components/`. The code program code lives in `app/app.c`.

## Setting Up
1. **Install Tools**:
  * [Git](https://git-scm.com/install/windows), for version control
  * [VS Code](https://code.visualstudio.com/download), with the following extensions:
    * [STM32CubeIDE for VSCode](https://marketplace.visualstudio.com/items?itemName=stmicroelectronics.stm32-vscode-extension)
    * [Serial Monitor](https://marketplace.visualstudio.com/items?itemName=ms-vscode.vscode-serial-monitor)
  * [STM32 CLI Tools](https://www.st.com/en/development-tools/stm32cubeclt.html), for flashing firmware to the board
  * [CMake](https://cmake.org/download/), the build system we use

2. **Get the code**: Once you have access to the Github, run the following commands in powershell:  
```bash
git clone --recursive https://github.com/Queens-Rocket-Engineering-Team/fw-onboarding
cd fw-onboarding
git switch -c first-last          # your name
git push -u origin first-last
```

3. **Build**: Open the `fw-onboarding` folder in VSCode, and run the following commands:
```bash
cmake --preset Debug
cmake --build --preset Debug
```

4. **Flash and run**: Once you have a physical board, plug in an ST-Link and USB-C cable for serial and run:
```bash
cmake --build --preset Debug --target flash
```

If everything is installed correctly, you should see the LED blink!

Open the serial monitor at 115200 baud. You should see STINGER: ThreadX running and a stream of IMU readings.

## Tasks

1. **Read the template**

Open `app/app.c` and read it fully. Try to get an understanding of what it is trying to do. 
The template has two threads: one blinks the LED, and the other reads from the IMU. Each one is essentially a function with its own loop that runs forever. Some more info:
* Each thread has a **priority**. In ThreadX, a lower number means a thread is more important.
* The most important thread that's available to run always gets to use the processor.
* `tx_thread_sleep()` makes a thread wait, which lets other threads run. A thread that never sleeps or waits hogs the processor and starves everything less important.

Try changing how fast the LED blinks, so you've edited, built and run something yourself.

2. **Read the barometer**

The altimeter measures altitude using air pressure, which comes from the MS5611 barometer. Add a new thread that reads it and prints the pressure and temperature. Copy the shape of the IMU thread.

* `components/ms5611/ms5611.h` has an example of how to use the driver at the top.
* The I2C address is already defined in `app.c` as `BARO_I2C_ADDR`, and there's a commented-out `ms5611_t baro` ready for you.
* Call `ms5611_init()` at the top of your thread function, not in` tx_application_define()`. The driver sleeps while the sensor measures, and sleeping isn't allowed before the threads start.
Pressure comes out in pascals (around 100,000). Temperature comes out in hundredths of a degree: 2554 means 25.54 °C.

3. Fix the sharing problem

Run your code for a few minutes and watch the output closely. You'll probably see the occasional read error or missing line.

That's because both sensors share one I2C bus, and every thread shares the serial port. If one thread gets interrupted halfway through using the bus and another thread jumps in, they collide. This kind of bug is called a race condition, and it's sneaky because it only happens sometimes.

The fix is a mutex: a lock that only one thread can hold at a time. A thread takes the lock before using the bus and gives it back after. Look up `tx_mutex_create()`, `tx_mutex_get()` and `tx_mutex_put()`.

* Make one mutex for the I2C bus and one for the serial port (inside dbg()).
* Create mutexes in `tx_application_define()`, before the threads that use them.
* If you don't see any errors, try reading the sensors faster. Races are easier to catch when things happen more often.


4. **Calculate altitude**

Getting here is the main goal of this onboarding. See [Altimeter concepts](#altimeter-concepts) below for the formula.

1. When the board starts up, measure the pressure on the ground. This is your reference pressure, $P_0$.
2. In your barometer loop, use the formula to turn the current pressure into a height above the ground, and print it.
3. Carry the board up a flight of stairs (or even just lift it up in the air!) and watch the reading change.

- One pressure reading is a little noisy. Average a bunch of readings at startup to get a steadier $P_0$.
- Use kelvin for temperature: add 273.15 to °C.
- `printf("%f")` doesn't work on this board by default. Print the altitude in centimeters as a whole number instead.
- Put your altitude math in its own file, e.g. `app/altitude.c`, and add it to the build next to `app/app.c` in `cmake/threadx.cmake`

## Altimeter concepts
 
Air pressure is the weight of the air above you. As you go up, there's less air above you, so the pressure drops: about 12 Pa for every meter near the ground. The air also gets colder as you go up, by about 6.5 °C per kilometer. That rate is called the **lapse rate**.
 
Put those together and you get the formula for altitude from pressure:
 
$$h = \frac{T_0}{L} \left[ \left( \frac{P}{P_0} \right)^{\frac{-R \cdot L}{g_0 \cdot M}} - 1 \right]$$
 
| Symbol | What it is | Value |
|---|---|---|
| $h$ | Height above where you measured $P_0$ | meters |
| $P$ | Pressure right now | Pa |
| $P_0$ | Pressure on the ground, measured at startup | Pa |
| $T_0$ | Temperature on the ground | **kelvin** |
| $L$ | Lapse rate | −0.0065 K/m |
| $R$ | Gas constant | 8.3144626 J/(mol·K) |
| $g_0$ | Gravity | 9.80665 m/s² |
| $M$ | Molar mass of air | 0.0289644 kg/mol |
 
Plugging in the constants, it simplifies to:
 
$$h = \frac{T_0}{0.0065} \left[ 1 - \left( \frac{P}{P_0} \right)^{0.190266} \right]$$
 
To check your code: with $P_0$ = 101325 Pa and $T_0$ = 288.15 K, a pressure of 100129.4 Pa should give 100 m, and 89874.6 Pa should give 1000 m.
 
This formula is for the lowest layer of the atmosphere (up to about 11 km), which is plenty for us.


## Stretch goals
 
Finished the core task and want more? Pick any of these.
 
- **Detect flight events.** Make the altimeter figure out what's happening: sitting on the pad, launched, reached the top (apogee), coming down, landed. Draw the states on paper first. Watch out for things that could fool it, like someone bumping the rocket on the pad.
- **Survive losing power.** If the board resets mid-flight, it shouldn't think it's back on the pad. Save the important state somewhere that survives a reset (the chip's flash memory) and load it on startup.
- **Stop printing from the sensor threads.** Printing is slow, and it slows down your sensor readings. Have the sensor threads send their readings to another thread through a *message queue* (`tx_queue_send()` / `tx_queue_receive()`), and do the printing there.

## Reference
 
**The board (STINGER V2.0)**
 
| | |
|---|---|
| Processor | STM32F103: 72 MHz, 128 KB flash, 20 KB RAM |
| Barometer | MS5611 on I2C2 |
| IMU | MPU6050 (accelerometer + gyro) on I2C2 |
| Serial output | USART1, 115200 baud |
| LED | PA15 |
| Pyro outputs | `FIRE_DROGUE` PA1, `FIRE_MAIN` PA3. **Don't use. See the safety note above.** |
 
**Where things are**
 
- `app/app.c`: your code
- `components/`: the sensor drivers
- `Core/`: setup code generated by STM32CubeMX (you won't need to edit it)
- `threadx/`: the RTOS itself
**Links**
 
- [ThreadX docs](https://threadx.io/releases/6.5.1/threadx/main/chapter4.html): look up any `tx_` function here
- [MS5611 datasheet](https://www.te.com/en/product-MS5611-01BA03.html)
- [MPU6050 datasheet](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf)