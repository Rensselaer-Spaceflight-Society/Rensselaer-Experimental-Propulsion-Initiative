cmake_minimum_required(VERSION 3.22)
# Enable CMake support for ASM and C languages
enable_language(C ASM)
# STM32CubeMX generated symbols (macros)
set(MX_Defines_Syms 
	USE_NUCLEO_64 
	CORE_CM4 
	USE_HAL_DRIVER 
	STM32WL55xx
    $<$<CONFIG:Debug>:DEBUG>
)

# STM32CubeMX generated include paths
set(MX_Include_Dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../SubGHz_Phy/App
    ${CMAKE_CURRENT_SOURCE_DIR}/../../SubGHz_Phy/Target
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/trace/adv_trace
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Inc/Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/misc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/sequencer
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/timer
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/lpm/tiny_lpm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/BSP/STM32WLxx_Nucleo
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/Device/ST/STM32WLxx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/SubGHz_Phy/radio_driver
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/Include
)

# STM32CubeMX generated application sources
set(MX_Application_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../SubGHz_Phy/Target/radio_board_if.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../SubGHz_Phy/App/app_subghz_phy.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../SubGHz_Phy/App/subghz_phy_app.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/stm32_lpm_if.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/sys_app.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/timer_if.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/stm32_adv_trace_if.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/stm32wlxx_it.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/stm32wlxx_hal_msp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/sysmem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/syscalls.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../startup_stm32wl55xx_cm4.s
)

# STM32 HAL/LL Drivers
set(STM32_Drivers_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/system_stm32wlxx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_rcc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_rcc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_flash_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_dma_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_pwr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_pwr_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_cortex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_exti.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_i2c.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_i2c_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_spi_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_subghz.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_tim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_tim_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_uart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_uart_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_usart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_usart_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/BSP/STM32WLxx_Nucleo/stm32wlxx_nucleo.c
)

# Drivers Midllewares


set(Utilities_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/trace/adv_trace/stm32_adv_trace.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/misc/stm32_mem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/misc/stm32_systime.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/misc/stm32_tiny_sscanf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/misc/stm32_tiny_vsnprintf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/sequencer/stm32_seq.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/timer/stm32_timer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/lpm/tiny_lpm/stm32_lpm.c
)
set(SubGHz_Phy_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/SubGHz_Phy/radio_driver/lr_fhss_mac.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/SubGHz_Phy/radio_driver/radio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/SubGHz_Phy/radio_driver/radio_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/SubGHz_Phy/radio_driver/radio_fw.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/SubGHz_Phy/radio_driver/wl_lr_fhss.c
)

# Link directories setup
set(MX_LINK_DIRS

)
# Project static libraries
set(MX_LINK_LIBS 
    STM32_Drivers
    ${TOOLCHAIN_LINK_LIBRARIES}
    Utilities	SubGHz_Phy	
)
# Interface library for includes and symbols
add_library(stm32cubemx INTERFACE)
target_include_directories(stm32cubemx INTERFACE ${MX_Include_Dirs})
target_compile_definitions(stm32cubemx INTERFACE ${MX_Defines_Syms})

# Create STM32_Drivers static library
add_library(STM32_Drivers OBJECT)
target_sources(STM32_Drivers PRIVATE ${STM32_Drivers_Src})
target_link_libraries(STM32_Drivers PUBLIC stm32cubemx)


# Create Utilities static library
add_library(Utilities OBJECT)
target_sources(Utilities PRIVATE ${Utilities_Src})
target_link_libraries(Utilities PUBLIC stm32cubemx)

# Create SubGHz_Phy static library
add_library(SubGHz_Phy OBJECT)
target_sources(SubGHz_Phy PRIVATE ${SubGHz_Phy_Src})
target_link_libraries(SubGHz_Phy PUBLIC stm32cubemx)

# Add STM32CubeMX generated application sources to the project
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${MX_Application_Src})

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE ${MX_LINK_DIRS})

# Add libraries to the project
target_link_libraries(${CMAKE_PROJECT_NAME} ${MX_LINK_LIBS})

# Add the map file to the list of files to be removed with 'clean' target
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_PROJECT_NAME}.map)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()
