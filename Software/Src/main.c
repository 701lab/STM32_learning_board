//#define __SOFT_FP__
//
//
//#if !defined(__SOFT_FP__) && defined(__ARM_FP)
//  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
//#endif

#define VAR_DECLS // Global variables initialization

#include "device.h"

void dummy_delay(uint32_t delay_time);

uint32_t system_counter = 0;

int16_t adc_potentiometers_data[3] = {0, 0, 0};
int16_t adc_all_sources_data[6] = {0, 0, 0, 0, 0, 0};

float currents[2] = {0.0f, 0.0f};

// Current sensors declarations
current_sensor ina240;
current_sensor acs711;

// Motor declaration
motor motor1 =
			{
					.encoder_constant = 2652.0f, //1200.0f, // don't know for sure
					.max_duty_cycle_coefficient = PWM_PRECISION
			};

speed_control motor1_speed_cotroller =
			{
					.current_integral = 0.0f,
					.controller_output_limitation_value = PWM_PRECISION,
					.previous_encoder_counter_value = 0,
					.previous_speed_mistake = 0.0f,
					.target_speed = 0.0f,
					.regulator_control_signal = 0.0f,
					.current_speed = 0.0f
			};

float motor_current_speed = 0.0f;
uint16_t current_encoder_counter_calue = 0;

int main(void)
{
	// *** Current sensors setup *** //
	ina240.zero_value = 2394;
	ina240.scale_factor = 0.00362585f; //0.0036621f; // A/(LSB) = (Vref / 2^(ADC precision in bits)) / (amplifier V/V * current shunt resistance)

	acs711.zero_value = 2159;
	acs711.scale_factor = 0.0086677f;


	// ****** Motor  initialization ****** //
	motor1.motor_disable = drv8701_nsleep_low;
	motor1.motor_enable = drv8701_nsleep_high;
	motor1.set_pwm_duty_cycle = set_motor_pwm;
	motor1.get_encoder_counter_value = get_motor_encoder_value;
	motor1.speed_controller = &motor1_speed_cotroller;
//	motor1.position_controller = &motor1_position_controller;
	motor1_speed_cotroller.kp = 200.0f;
	motor1_speed_cotroller.ki = 5000.0f;
//	motor1_position_controller.kp = 1.0f;
//	motor1_position_controller.position_precision = 8.0f/motor1.encoder_constant;



	full_device_setup(no, no);
	adc_2_setup(adc_all_sources_data);

	// *** работа с двигателем постоянного тока *** //

	// Enable motor
	drv8701_nsleep_high(); // enable motor driver

	// Должна вызываться при 0 токе в датчике. ДОлжна вызываться только в режиме отладки
//	calibrate_current_sencors(&acs711_zero, &ina240_zero, adc_all_sources_data);

//	basic_spi3_setup(5000000);
//	basic_uart2_setup(19200);

	while (1)
	{

//		current_encoder_counter_calue = TIM3->CNT;
//		delay_in_milliseconds(200);


		// Speed controller test
		delay_in_milliseconds(2000);
		GPIOB->ODR ^= 0x60;
		motor1.speed_controller->target_speed = 0.0f;

		delay_in_milliseconds(2000);
		GPIOB->ODR ^= 0x60;
		motor1.speed_controller->target_speed = 0.5f;

		delay_in_milliseconds(2000);
		GPIOB->ODR ^= 0x60;
		motor1.speed_controller->target_speed = 1.0f;

		delay_in_milliseconds(2000);
		GPIOB->ODR ^= 0x60;
		motor1.speed_controller->target_speed = 1.5f;

		delay_in_milliseconds(2000);
		GPIOB->ODR ^= 0x60;
		motor1.speed_controller->target_speed = 1.0f;

		delay_in_milliseconds(2000);
		GPIOB->ODR ^= 0x60;
		motor1.speed_controller->target_speed = 0.5f;

		delay_in_milliseconds(2000);
		GPIOB->ODR ^= 0x60;
		motor1.speed_controller->target_speed = 0.0f;

		delay_in_milliseconds(2000);
		GPIOB->ODR ^= 0x60;
		motor1.speed_controller->target_speed = -0.5f;

		delay_in_milliseconds(2000);
		GPIOB->ODR ^= 0x60;
		motor1.speed_controller->target_speed = -1.0f;

		delay_in_milliseconds(2000);
		GPIOB->ODR ^= 0x60;
		motor1.speed_controller->target_speed = -1.5f;

		delay_in_milliseconds(2000);
		GPIOB->ODR ^= 0x60;
		motor1.speed_controller->target_speed = -1.0f;

		delay_in_milliseconds(2000);
		GPIOB->ODR ^= 0x60;
		motor1.speed_controller->target_speed = -0.5f;
		// Raw pwm motor test


//		// Тест с испльзованием просто ШИМ. При этом тесте НУЖНО ОБЯЗАТЕЛЬНО ОТКЛЮЧАТЬ ОБРАБОТКУ РЕГУЛЯТОРА СКОРОСТИ
//		delay_in_milliseconds(2000);
//		GPIOB->ODR ^= 0x60;
//		set_motor_pwm(PWM_PRECISION / 3 * 2);
//
//		delay_in_milliseconds(2000);
//		GPIOB->ODR ^= 0x60;
//		set_motor_pwm(PWM_PRECISION);
//
//		delay_in_milliseconds(2000);
//		GPIOB->ODR ^= 0x60;
//		set_motor_pwm(PWM_PRECISION / 3 * 2);
//
//		delay_in_milliseconds(2000);
//		GPIOB->ODR ^= 0x60;
//		set_motor_pwm(PWM_PRECISION / 3);
//
//		delay_in_milliseconds(2000);
//		GPIOB->ODR ^= 0x60;
//		set_motor_pwm(0);
//
//		delay_in_milliseconds(2000);
//		GPIOB->ODR ^= 0x60;
//		set_motor_pwm(PWM_PRECISION / (-3));
//
//		delay_in_milliseconds(2000);
//		GPIOB->ODR ^= 0x60;
//		set_motor_pwm(PWM_PRECISION / (-3) * 2);
//
//		delay_in_milliseconds(2000);
//		GPIOB->ODR ^= 0x60;
//		set_motor_pwm((-1) * PWM_PRECISION);
//
//		delay_in_milliseconds(2000);
//		GPIOB->ODR ^= 0x60;
//		set_motor_pwm(PWM_PRECISION / (-3) * 2);
//
//		delay_in_milliseconds(2000);
//		GPIOB->ODR ^= 0x60;
//		set_motor_pwm( PWM_PRECISION / (-3));
//
//		delay_in_milliseconds(2000);
//		GPIOB->ODR ^= 0x60;
//		set_motor_pwm(0);



//		adc_2_manually_get_data();
//		currents[0] = calculate_current(&ina240, adc_all_sources_data[adc_ina_cs]);
//		currents[1] = calculate_current(&acs711, adc_all_sources_data[adc_acs_cs]);

//		spi3_write_single_byte(153);
//		uart2_send_byte(54);


//		GPIOB->ODR ^= 0x60;
//		dummy_delay(1000000);
	}

//	// Basic button test
//	while (1)
//	{
//		GPIOB->ODR &= ~0x78;
//		GPIOB->ODR |= (GPIOC->IDR & 0xE000) >> 9;
//	}

//	// Pure LED setup test
//	uint32_t led_number = 0;
//
//	while (1)
//	{
//		GPIOB->ODR ^= 1 << (led_number + 3);
//		led_number += 1;
//		if(led_number > 3){
//			led_number = 0;
//		}
//
//		dummy_delay(100000);
//	}

}

// ****** Control loop handler ****** //
// Control loop handles by the SysTick timer

// *** Speed controller setup variables ***//
uint32_t speed_loop_call_counter = 0;

#define SPEED_LOOP_FREQUENCY				20	// Times per second. Must be not bigger then SYSTICK_FREQUENCY.
#define SPEED_LOOP_COUNTER_MAX_VALUE 		(SYSTICK_FREQUENCY / SPEED_LOOP_FREQUENCY)	// Times.
#define SPEED_LOOP_PERIOD					(float)(SPEED_LOOP_FREQUENCY) / (float)(SYSTICK_FREQUENCY) // Seconds.


void SysTick_Handler()
{

	speed_loop_call_counter += 1;
	if ( speed_loop_call_counter == SPEED_LOOP_COUNTER_MAX_VALUE )	// 20 times per second
	{
		speed_loop_call_counter = 0;
		motor_current_speed = motors_get_speed_by_incements(&motor1, SPEED_LOOP_PERIOD);
		float m1_voltage_task = motors_speed_controller_handler(&motor1, SPEED_LOOP_PERIOD);
		motor1.set_pwm_duty_cycle((int32_t)m1_voltage_task);
	}

	system_counter++;
	if(system_counter >= SYSTICK_FREQUENCY / 2)
	{
		GPIOB->ODR ^= 0x08;
		system_counter = 0;
	}

	adc_2_manually_get_data();
	currents[0] = calculate_current(&ina240, adc_all_sources_data[adc_ina_cs]);
	currents[1] = calculate_current(&acs711, adc_all_sources_data[adc_acs_cs]);

}


void dummy_delay(uint32_t delay_time)
{
	for(uint32_t i = 0; i < delay_time; ++i)
	{

	}
}


