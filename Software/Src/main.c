//#if !defined(__SOFT_FP__) && defined(__ARM_FP)
//  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
//#endif

#define VAR_DECLS // Global variables initialization

#include "device.h"

void dummy_delay(uint32_t delay_time);

int main(void)
{

	full_device_setup(no, no);

//	while (1)
//	{
//
//	}

//	// Basic button test
//	gpio_setup();
//	while (1)
//	{
//		GPIOB->ODR &= ~0x78;
//		GPIOB->ODR |= (GPIOC->IDR & 0xE000) >> 9;
//	}

	// Pure LED setup test
	gpio_setup();
	uint32_t led_number = 0;
	while (1)
	{
		GPIOB->ODR ^= 1 << (led_number + 3);
		led_number += 1;
		if(led_number > 4){
			led_number = 0;
		}

		dummy_delay(100000);
	}

}

void dummy_delay(uint32_t delay_time)
{
	for(uint32_t i = 0; i < delay_time; ++i)
	{

	}
}

