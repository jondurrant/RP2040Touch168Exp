#include "LCD_test.h"  //example
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"


#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>
#include <rmw_microros/rmw_microros.h>

#include "pico_uart_transport.h"



void core1_entry() {
	LCD_1in69_LVGL_Test();
}

rcl_publisher_t publisher;
std_msgs__msg__Int32 msg;

void timer_callback(rcl_timer_t *timer, int64_t last_call_time)
{
    rcl_ret_t ret = rcl_publish(&publisher, &msg, NULL);
    msg.data++;
}


void uRos(){
	rmw_uros_set_custom_transport(
		true,
		NULL,
		pico_serial_transport_open,
		pico_serial_transport_close,
		pico_serial_transport_write,
		pico_serial_transport_read
	);

	rcl_timer_t timer;
	rcl_node_t node;
	rcl_allocator_t allocator;
	rclc_support_t support;
	rclc_executor_t executor;

	allocator = rcl_get_default_allocator();

	// Wait for agent successful ping for 2 minutes.
	const int timeout_ms = 1000;
	const uint8_t attempts = 120;

	rcl_ret_t ret = rmw_uros_ping_agent(timeout_ms, attempts);

	if (ret != RCL_RET_OK)
	{
		// Unreachable agent, exiting program.
		return ret;
	}

	rclc_support_init(&support, 0, NULL, &allocator);

	rclc_node_init_default(&node, "pico_node", "", &support);
	rclc_publisher_init_default(
		&publisher,
		&node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
		"pico_publisher");

	rclc_timer_init_default(
		&timer,
		&support,
		RCL_MS_TO_NS(1000),
		timer_callback);

	rclc_executor_init(&executor, &support.context, 1, &allocator);
	rclc_executor_add_timer(&executor, &timer);


	msg.data = 0;
	for (;;){
		rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
	}
}



int main(void)
{


	multicore_launch_core1(core1_entry);

	uRos();

	for (;;){
		sleep_ms(3000);
	}
}
