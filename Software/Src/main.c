//#if !defined(__SOFT_FP__) && defined(__ARM_FP)
//  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
//#endif

#define VAR_DECLS // Global variables initialization

#include "device.h"

void dummy_delay(uint32_t delay_time);

uint32_t system_counter = 0;

int main(void)
{

	full_device_setup(no, no);

	basic_spi3_setup(5000000);

//	basic_uart2_setup(19200);


	while (1)
	{


		delay_in_milliseconds(200);

		GPIOB->ODR ^= 0x60;
		spi3_write_single_byte(153);
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


