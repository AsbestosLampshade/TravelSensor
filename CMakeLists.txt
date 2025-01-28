
add_executable(press_temp
    press_temp.c
    )

# pull in common dependencies
target_link_libraries(press_temp pico_stdlib hardware_i2c)

pico_enable_stdio_usb(press_temp 1)
pico_enable_stdio_uart(press_temp 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(press_temp)

