/*
MIT License

Copyright (c) 2024 Society of Robotics and Automation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "sra_board.h"

#include <stdio.h>
#include <math.h>

#include "tuning_http_server.h"

#define TAG "MCPWM_SERVO_CONTROL"

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


static void mcpwm_servo_control(void *arg)
{
	enable_servo();
	bool morph_lock = false;
	bool morph_unlock = false;
	while (1)
	{   /*ESP_LOGI(TAG, "%d", read_servo_const().morph);*/
		if (read_servo_const().servo_a != 0){
			
			morph_unlock = false;
				if (morph_lock){
					vTaskDelay(10);
					continue;
				}
				else {
					for (int i = 1; i <= 90 ; i++)
				{	
					ESP_LOGI(TAG, "Morphing...\n");
					set_angle_servo(&servo_a, i);
					set_angle_servo(&servo_b, 360-i);
					set_angle_servo(&servo_c, 360-i);
					set_angle_servo(&servo_d, i);
					vTaskDelay(10);
					if(i == 90){
						morph_lock = true;
						ESP_LOGI(TAG, "Drone Mode\n");
					}
				}
			}
		}

			else if ( read_servo_const().servo_a==0){

			morph_lock = false; 

			if (morph_unlock){
				vTaskDelay(10);
				continue;
			}
			else{
				for (int i = 90; i >= 0; i--)
				{	
					ESP_LOGI(TAG, "Demorphing...\n");
					set_angle_servo(&servo_a, i);
					set_angle_servo(&servo_b, 360-i);
					set_angle_servo(&servo_c, 360-i);
					set_angle_servo(&servo_d, i);
					vTaskDelay(10);
					if (i == 0){
						morph_unlock = true;
						ESP_LOGI(TAG, "Quad Mode\n");
					}

				}
			}
		}
		else {
			set_angle_servo(&servo_a, 0);
			set_angle_servo(&servo_b, 0);
			set_angle_servo(&servo_c, 0);
			set_angle_servo(&servo_d, 0);
		}
		vTaskDelay(100);
	}
}

void app_main()
{
	ESP_LOGD(TAG, "Testing servo motors\n");
	xTaskCreate(mcpwm_servo_control, "mcpwm_example_servo_control", 4096, NULL, 5, NULL);
	vTaskDelay(1000);
	start_tuning_http_server();
}
