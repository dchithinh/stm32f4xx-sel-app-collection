"""
PlatformIO pre-script: name link outputs after this app (same basename as CubeIDE Release/).
"""
Import("env")

# Was: stm32f407xx_pomodoro.bin from STM32CubeIDE Release build
env.Replace(PROGNAME="stm32f407xx_pomodoro")
