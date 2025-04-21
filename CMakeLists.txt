cmake_minimum_required(VERSION 3.13)

# Definições do projeto
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)

project(Projeto_Integrado C CXX ASM)

pico_sdk_init()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)

add_executable(Projeto_Integrado
    Projeto_Integrado.c       # Código principal
    inc/ssd1306.c       # Driver do OLED SSD1306
)

pico_generate_pio_header(Projeto_Integrado ${CMAKE_CURRENT_LIST_DIR}/ws2818b.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)

# Habilitar saída via UART e USB
pico_enable_stdio_uart(Projeto_Integrado 1)
pico_enable_stdio_usb(Projeto_Integrado 1)

# Vincular bibliotecas necessárias
target_link_libraries(Projeto_Integrado
    PRIVATE
    pico_stdlib
    hardware_pwm
    hardware_adc
    hardware_timer
    hardware_pio  # Para controlar o PIO
    hardware_i2c  # Para comunicação com o SSD1306
)

# Gerar arquivos binários extras (HEX, UF2)
pico_add_extra_outputs(Projeto_Integrado)
