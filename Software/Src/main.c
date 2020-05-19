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


int main(void)
{
	ina240.zero_value = 2394;
	ina240.scale_factor = 0.0036621f; // A/(LSB) = (Vref / 2^(ADC precision in bits)) / (amplifier V/V * current shunt resistance)

	acs711.zero_value = 2159;
	acs711.scale_factor = 0.0086677f;

	full_device_setup(no, no);
	adc_2_setup(adc_all_sources_data);

	// Должна вызываться при 0 токе в датчике. ДОлжна вызываться только в режиме отладки
//	calibrate_current_sencors(&acs711_zero, &ina240_zero, adc_all_sources_data);

//	basic_spi3_setup(5000000);
//	basic_uart2_setup(19200);

	while (1)
	{
		delay_in_milliseconds(200);

		GPIOB->ODR ^= 0x60;

		adc_2_manually_get_data();
		currents[0] = calculate_current(&ina240, adc_all_sources_data[adc_ina_cs]);
		currents[1] = calculate_current(&acs711, adc_all_sources_data[adc_acs_cs]);

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

void dummy_delay(uint32_t delay_time)
{
	for(uint32_t i = 0; i < delay_time; ++i)
	{

	}
}


void SysTick_Handler()
{
	system_counter++;
	if(system_counter >= SYSTICK_FREQUENCY / 2)
	{
		GPIOB->ODR ^= 0x08;
		system_counter = 0;
	}
}


