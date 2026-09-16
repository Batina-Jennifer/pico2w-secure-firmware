cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2_w)
set(PICO_PLATFORM rp2350-arm-s)

#Pulling Standard Raspberry Pi Pico SDK helper script.
include(pico_sdk_import.cmake) 

#Target project name and language standards.
project(pico2w_secure_firmware C CXX ASM)

pico_sdk_init()

#Binary executable target for the secure boot application.
add_executable(secure_boot_app
    main.c
)

target_include_directories(secure_boot_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(secure_boot_app 
    pico_stdlib
    pico_mbedtls
)  

#Enable USB output and disable UART output.
pico_enable_stdio_usb(secure_boot_app 1)
pico_enable_stdio_uart(secure_boot_app 0)

#Generate extra output formats (.uf2, .elf, .bin, .hex)
pico_add_extra_outputs(secure_boot_app)