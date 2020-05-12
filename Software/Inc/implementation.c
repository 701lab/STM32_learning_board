#include "implementation.h"


// ******** MCU connections overview ******** //
/*
	LEDs:
		PB3-PB6 (digital outputs).

	Motor Controller ( DRV8848 ):
		PC6 - nSleep (digital output) - Logic high to enable device, logic low to enter low-power sleep mode;
		PC7 - nFault (digital input with interrupt) - Pulled logic low with fault condition;
		PA8-PA9 - AIN1-2 (TIM1 PWM CH1-2, alternate function 2) - motor 2 speed control inputs;
		PA10-PA11 - BIN2-1 (TIM1 PWM CH3-4, alternate function 4) - motor 1 speed control inputs.

	Radio module ( NRF24l01+ ):
		PA5-PA7 - SCK, MISO, MOSI respectively (SPI1, alternate function 0);
		PB0 - CE (digital output) - chip enable, logic high to turn on device;
		PB1 - CSN (digital output) - SPI chip select;
		PB2 - IRQ (interrupt input) - Pulled logic low with interrupt condition.

	Motor 1 encoder:
		PB3 - encoder input 2 (TIM2_CH2, alternate function 2),
		PA15 - encoder input 1 (TIM2_CH1, alternate function 2).

	Motor 2 encoder:
		PB4-5 encoder input 1-2 (TIM3 CH1-2, alternate function 1).

	IMU ( ICM-20600 ):
		PB13-PB15 (SPI2 SCK, MISO, MOSI, alternate function 0), PB10-PB11 (INT2, INT1, respectively, external interrupt), PB12 (CS, digital output).

	Voltage control:
		PA0 - input voltage ADC (ADC1 IN0, analog input).

	USART:
		PB6-7 - TX and RX respectively (USART1, alternate function 0).

	TIM14 - counts time with 0.1 ms precision for mistakes log

	TIM15 - high speed counter for precise speed calculations.

	TIM16 - timer for proper delay implementation

 */


