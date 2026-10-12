add_library(components OBJECT
    ms5611/ms5611.c
    ms5611/ms5611_calc.c
    mpu6050/mpu6050.c
    mpu6050/mpu6050_calc.c
)

target_include_directories(components PUBLIC
    ms5611
    mpu6050
)

# The driver still uses HAL and ThreadX directly, so it needs their
# headers and compile definitions (STM32F103xB, USE_HAL_DRIVER, ...).
target_link_libraries(components PRIVATE
    stm32cubemx
    threadx
)