//#if !defined(__SOFT_FP__) && defined(__ARM_FP)
//  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
//#endif

#define VAR_DECLS // Global variables initialization

#include "device.h"

void dummy_delay(uint32_t delay_time);

uint32_t system_counter = 0;

int main(void)
{

//	system_clock_setup();

//	gpio_setup();

	full_device_setup(no, no);

	while (1)
	{
//		GPIOB->ODR ^= 0x60;
		dummy_delay(1000000);
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


