cmake_minimum_required(VERSION 3.20.0)

set(BOARD stm32f4_disco)
set(CONF_FILE prj.conf)
find_package(Zephyr)
project(my_zephyr_app)

target_sources(app PRIVATE
        src/audio.cpp
        src/leds.cpp
        src/usb.c
        src/Switch.cpp
        src/key.cpp
        src/i2c.c
        src/peripherals.cpp
        src/synth.cpp
        src/main.cpp
)
