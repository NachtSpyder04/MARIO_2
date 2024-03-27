#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <math.h>

#include <uros_network_interfaces.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <geometry_msgs/msg/twist.h>

#include <driver/gpio.h>
#include <driver/ledc.h>

#include <rmw_microros/rmw_microros.h>

#include "sra_board.h"

#define TAG "MCPWM_SERVO_CONTROL"

// Macro functions
#define constrain(amt, low, high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Aborting.\n",__LINE__,(int)temp_rc);vTaskDelete(NULL);}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Continuing.\n",__LINE__,(int)temp_rc);}}


// Constants
#define FRAME_TIME 100 // 1000 / FRAME_TIME = FPS
#define SLEEP_TIME 10

// Constants
#define FRAME_TIME 100 // 1000 / FRAME_TIME = FPS
#define SLEEP_TIME 10

#define TAG_RVIZ "MICROROS_RVIZ"
#define pi 3.141592653589


geometry_msgs__msg__Twist msg;

servo_config servo_a = {
	.servo_pin = SERVO_A,
	.min_pulse_width = CONFIG_SERVO_A_MIN_PULSEWIDTH,
	.max_pulse_width = CONFIG_SERVO_A_MAX_PULSEWIDTH,
	.max_degree = CONFIG_SERVO_A_MAX_DEGREE,
	.mcpwm_num = MCPWM_UNIT_0,
	.timer_num = MCPWM_TIMER_0,
	.gen = MCPWM_OPR_A,
};

servo_config servo_b = {
	.servo_pin = SERVO_B,
	.min_pulse_width = CONFIG_SERVO_B_MIN_PULSEWIDTH,
	.max_pulse_width = CONFIG_SERVO_B_MAX_PULSEWIDTH,
	.max_degree = CONFIG_SERVO_B_MAX_DEGREE,
	.mcpwm_num = MCPWM_UNIT_0,
	.timer_num = MCPWM_TIMER_0,
	.gen = MCPWM_OPR_B,
};

servo_config servo_c = {
	.servo_pin = SERVO_C,
	.min_pulse_width = CONFIG_SERVO_C_MIN_PULSEWIDTH,
	.max_pulse_width = CONFIG_SERVO_C_MAX_PULSEWIDTH,
	.max_degree = CONFIG_SERVO_C_MAX_DEGREE,
	.mcpwm_num = MCPWM_UNIT_0,
	.timer_num = MCPWM_TIMER_1,
	.gen = MCPWM_OPR_A,
};

servo_config servo_d = {
	.servo_pin = SERVO_D,
	.min_pulse_width = CONFIG_SERVO_D_MIN_PULSEWIDTH,
	.max_pulse_width = CONFIG_SERVO_D_MAX_PULSEWIDTH,
	.max_degree = CONFIG_SERVO_D_MAX_DEGREE,
	.mcpwm_num = MCPWM_UNIT_0,
	.timer_num = MCPWM_TIMER_1,
	.gen = MCPWM_OPR_B,
};

// Function forward declarations
void setupRos();
void cmd_vel_callback(const void *msgin);
void timer_callback(rcl_timer_t *timer, int64_t last_call_time);
float fmap(float val, float in_min, float in_max, float out_min, float out_max);

void micro_ros_task(void * arg)
{
   setupRos(); 
}

void app_main(void)
{
#if defined(CONFIG_MICRO_ROS_ESP_NETIF_WLAN) || defined(CONFIG_MICRO_ROS_ESP_NETIF_ENET)
    ESP_ERROR_CHECK(uros_network_interface_initialize());
#endif
    enable_servo();
    //pin micro-ros task in APP_CPU to make PRO_CPU to deal with wifi:
     printf("Angular");
    xTaskCreate(micro_ros_task,
            "uros_task",
            CONFIG_MICRO_ROS_APP_STACK,
            NULL,
            CONFIG_MICRO_ROS_APP_TASK_PRIO,
            NULL);
}

void setupRos() {
    // Micro ROS
      printf("Angular");
    rcl_allocator_t allocator = rcl_get_default_allocator();
    rclc_support_t support;

// Create init_options.
	rcl_init_options_t init_options = rcl_get_zero_initialized_init_options();
	RCCHECK(rcl_init_options_init(&init_options, allocator));

#ifdef CONFIG_MICRO_ROS_ESP_XRCE_DDS_MIDDLEWARE
	rmw_init_options_t* rmw_options = rcl_init_options_get_rmw_init_options(&init_options);

	// Static Agent IP and port can be used instead of autodisvery.
	RCCHECK(rmw_uros_options_set_udp_address(CONFIG_MICRO_ROS_AGENT_IP, CONFIG_MICRO_ROS_AGENT_PORT, rmw_options));
	//RCCHECK(rmw_uros_discover_agent(rmw_options));
#endif
	// Setup support structure.
	RCCHECK(rclc_support_init_with_options(&support, 0, NULL, &init_options, &allocator));

    // create node
    rcl_node_t node;
    RCCHECK(rclc_node_init_default(&node, "ros_esp32cam_diffdrive", "", &support));

    // create subscriber
    rcl_subscription_t subscriber;
    RCCHECK(rclc_subscription_init_default(
        &subscriber,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
        "/joint_states")); //Subscribe topic here on which you are publishing

    // create timer,
    rcl_timer_t timer;
    RCCHECK(rclc_timer_init_default(
        &timer,
        &support,
        RCL_MS_TO_NS(FRAME_TIME),
        timer_callback));

    // create executor
    rclc_executor_t executor;
    RCCHECK(rclc_executor_init(&executor, &support.context, 2, &allocator));
    RCCHECK(rclc_executor_add_subscription(&executor, &subscriber, &msg, &cmd_vel_callback, ON_NEW_DATA));
    RCCHECK(rclc_executor_add_timer(&executor, &timer));

    while (1) {
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(SLEEP_TIME));
        usleep(SLEEP_TIME * 1000);
    }

    // free resources
    RCCHECK(rcl_subscription_fini(&subscriber, &node));
    RCCHECK(rcl_node_fini(&node));

    vTaskDelete(NULL);
}

// We don't really need the callback, because msg is set anyway
void joint_states_callback(const void *msgin) {
//    const geometry_msgs__msg__Twist *msg = (const geometry_msgs__msg__Twist *) msgin;
//    printf("Message received: %f %f\n", msg->linear.x, msg->angular.z);
}

// Each frame, check msg data and set PWM channels accordingly

/*
IMPLEMENT YOUR LOGIC IN THIS timer_callback FUNCTION. 
BY DEFAULT THIS WILL RUN IN INFINITE WHILE LOOP.
ACCES THE DATA FROM TOPIC(ON WHICH YOU PUBLISHED).
for ex:  float linear = msg.linear.x;
         int angular = msg.angular.z; 
*/
void timer_callback(rcl_timer_t *timer, int64_t last_call_time) {

    if (timer == NULL) {
        return;
    }

    ESP_LOGD(TAG_RVIZ, "%s", "new message from publisher");

    ESP_LOGD(TAG_RVIZ, "timestamp:        %d", msg.header.seq);
    ESP_LOGD(TAG_RVIZ, "angle [BASE}:     %f", msg.position[0]);
    ESP_LOGD(TAG_RVIZ, "angle [SHOUDLER]: %f", msg.position[1]);
    ESP_LOGD(TAG_RVIZ, "angle [ELBOW]:    %f", msg.position[2]);
    ESP_LOGD(TAG_RVIZ, "angle [GRIPPER]:    %f", msg.position[3]);

    set_angle_servo(&servo_a,msg.position[0]*180/pi);
    set_angle_servo(&servo_b,msg.position[1]*180/pi);
    set_angle_servo(&servo_c,msg.position[2]*180/pi);
    set_angle_servo(&servo_d,msg.position[3]*180/pi);

    vTaskDelay(5);

}


// Helper functions
// -------------------------------------------------------------------

float fmap(float val, float in_min, float in_max, float out_min, float out_max) {
    return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
