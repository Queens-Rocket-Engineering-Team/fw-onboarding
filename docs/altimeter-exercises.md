# Firmware Onboarding
Firmware Onboarding via altimeter rewrite in Real Time Operating System format.

The goal of this module is to teach you how to write code for the firmware that will be on the rocket. We are moving toward an RTOS (Real Time Operating System) design, which requires a complete rewrite of the current code. This module will guide you through creating the altimeter code, giving you the concepts needed for altimeters in general, the requirements of our altimeter, and tips for how to program real time operating systems in C.

1. First, let's make a branch for your altimeter code.
  1. Go to the directory you want your altimeter code to live in.
      ```bash
      cd <directory-name>
      ```
  2. Clone and move into the fw-onboarding repository.
       ```bash
       git clone https://github.com/Queens-Rocket-Engineering-Team/fw-onboarding.git
       cd fw-onboarding
       ```
  3. Next, create a branch off of main, this will be where your altimeter code lives.
    1. Ensure you are in the Avionics team so you have permission to make branches. If you are not, ask any of the current Avionics leads.
    2. Ensure you are on main and then create a new branch with your name in the format "first-last".
      ```bash
      git switch main && git pull
      git switch -b yournamefirst-last
      ```
2. Now, make a file called "altimeter_module.c", or something similar (keeping the .c suffix.)
3. Given the resources below, create the altimeter code with a real time operating system! The main requirement is it needs to calculate the current altitude live.

Some suggestions for steps:
1. Include required libraries.
2. Set $P_{0}$ and $T_{0}$ at flight start.
3. Create a function that calculates the current altitude given the altitude formula in Altimeter Concepts below.

## Altimeter Concepts
The altimeter currently has a MS5611 pressure and temperature sensor, KX134 accelerometer, and MPU6050 gyroscope.

As we rise in the Earth's atmosphere, the density of air decreases and, as warm air rises, the air that remains is much warmer than below. The following image from pilotinstitute shows this in comparison to the internal pressure a typical airliner is kept to:

<img width="909" height="500" alt="image" src="https://github.com/user-attachments/assets/996de185-595a-4eab-851b-2421403aeaa2" />

The following is the formula for altitude given the current temperature and pressure of the air surrounding the rocket:

$$Altitude = \frac{T_{0}}{L} \left[ \left( \frac{P}{P_{0}} \right)^{\frac{-R \cdot L}{g_{0} \cdot M_{air}}} - 1 \right]$$

Where:
$L$ = -0.0065 $K/m$ (L is the Lapse Rate, the rate of temperature change as we increase in height. This can be considered a constant, at -0.0065 K/m (Kelvin per meter). KEEP ALL UNITS CONSISTENT, as this constant uses Kelvin, it is recommended to use Kelvin in your calculations, with the option to convert back to Celsius or Fahrenheit after for displaying the data as needed.)
$g_{0}$ = 9.8 $m/s^{2}$ (The acceleration due to gravity at an altitude of 0 meters where gravity is relative to the location of take-off and altitude is relative to sea level. We can just use 9.8 for simplicity)
$M_{air}$ = 0.02896 $kg$ (The average molar mass of Earth's air.)
$R \approx 8.3144626 J/(K \cdot mol)$ (Universal gas constant)
$P_{0}$ is the base pressure at launch. (In pascals as that is what the MS5611 measures in)
$T_{0}$ is the base temperature at launch. (MS5611 measures in Celsius, so make sure to add 273.15 when getting data from the device)

So, in simpler terms:

$$Altitude = \frac{T_{0}}{-0.0065} \left[ \left( \frac{P}{P_{0}} \right)^{0.19026627} - 1 \right]$$

This concept and formula only works best within the Troposphere, but for our purposes that's perfect.

## Stretch Goals
- State detection. The altimeter handles state transitions over the rockets journey.
- State storage and recovery. There is a chance the altimeter can lose power temporarily and turn back on. If this happens it needs to be able to remember which state it was in previously and cover edge cases of losing power when moving between states.

<!-- Currently unused images: -->
<!-- <img width="1010" height="253" alt="image" src="https://github.com/user-attachments/assets/8d5c2ace-5dfa-4c3e-a56a-822be3c5ad45" /><img width="1010" height="253" alt="image" src="https://github.com/user-attachments/assets/4dc03cfa-e6a0-471e-9507-565a536ef1b3" /> -->
