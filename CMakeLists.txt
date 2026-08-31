cmake_minimum_required(VERSION 3.13)

#Pulling Standard Raspberry Pi Pico SDK helper script.
include(pico_sdk_import.cmake OPTIONAL) 

#Target project name and language standards.
project(pico2w_secure_firmware C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

#Target Raspberry Pi Pico 2W board.
set(PICO_BOARD pico2_w)

pico_sdk_init()

#Binary executable target for the secure boot application.
add_executable(secure_boot_app
    main.c
)

#Enable USB output and disable UART output.
pico_enable_stdio_usb(secure_boot_app 1)
pico_enable_stdio_uart(secure_boot_app 0)

target_link_libraries(secure_boot_app 
    pico_stdlib
)  

#Generate extra output formats (.uf2, .elf, .bin, .hex)
pico_add_extra_outputs(secure_boot_app)