# Altimeter exercises

Start after the [ThreadX hardware checks](../README.md#flash-debug-and-check-the-scheduler)
pass. These are future exercises, not implemented features of the kernel example.
Write C source and headers and add them to the CMake build.

The team's altimeter design uses an MS5611 pressure/temperature sensor, a KX134
accelerometer, and an MPU6050 gyroscope. Confirm your board's wiring, buses, and
device configuration before writing drivers; the foundation does not configure
these peripherals.

## Pressure and altitude

1. Implement and test MS5611 acquisition, including calibration and compensation.
   Use pascals and kelvin at the calculation boundary; convert Celsius to kelvin
   by adding 273.15.
2. Capture reference pressure `P0` and reference temperature `T0` while stationary
   at the reference altitude.
3. Calculate altitude relative to that reference with the lapse-rate model:

   $$h = \frac{T_0}{L}\left[\left(\frac{P}{P_0}\right)^{-RL/(g_0 M)} - 1\right]$$

   | Symbol | Meaning | Example value / unit |
   | --- | --- | --- |
   | `P` | Current pressure | Pa |
   | `P0` | Reference pressure | Pa |
   | `T0` | Reference temperature | K |
   | `L` | Assumed temperature lapse rate | -0.0065 K/m |
   | `g0` | Assumed gravitational acceleration | 9.8 m/s² |
   | `M` | Molar mass of air | 0.02896 kg/mol |
   | `R` | Universal gas constant | 8.3144626 J/(K·mol) |

   The result is in metres. This is an approximation for the troposphere; weather
   and reference measurements affect the estimate. With a launch reference, the
   result is relative to launch, not automatically above sea level.
4. Test that `P == P0` yields zero altitude and decreasing pressure increases
   altitude. Handle invalid, nonpositive, and unavailable measurements.

## ThreadX application design

Give one thread ownership of acquisition and send complete samples through a
queue to the calculation thread. Replace the heartbeat bodies after choosing and
measuring suitable sampling periods. Keep interrupt handlers short and use
ThreadX waits instead of polling loops. Measure stack usage after adding drivers
and floating-point calculations; the 1 KiB stacks are initial sizes for the counter
example.

## Further exercises

- Detect flight states from validated measurements and test transitions with
  recorded or simulated sensor data.
- Investigate state recovery after resets, including resets during a transition
  or a storage write.
- Handle sensor errors, stale samples, and unavailable storage explicitly.

These exercises require their own hardware testing and requirements before use
as flight firmware.
