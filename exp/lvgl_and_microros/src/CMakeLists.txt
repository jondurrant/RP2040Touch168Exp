add_executable(${NAME}
        ImageData.c
        LCD_1in69_LVGL_test.c
        LVGL_example.c
        main.c
        pico_uart_transport.c
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(${NAME} 
	pico_stdlib
	Config 
	LCD 
	Touch 
	QMI8658 
	PCF85063A
	lvgl
	hardware_clocks 
	hardware_rtc 
	hardware_adc
	hardware_dma
	pico_multicore
	micro_ros
	
)

# create map/bin/hex file etc.
pico_add_extra_outputs(${NAME})

# enable usb output, disable uart output
pico_enable_stdio_usb(${NAME} 1)
pico_enable_stdio_uart(${NAME} 0)
add_compile_definitions(PICO_UART_ENABLE_CRLF_SUPPORT=0)
add_compile_definitions(PICO_STDIO_ENABLE_CRLF_SUPPORT=0)
add_compile_definitions(PICO_STDIO_DEFAULT_CRLF=0)

