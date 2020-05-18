#include "implementation.h"

// ******** MCU connections overview ******** //
/*
	LEDs:
		PB3-PB6 (digital outputs);

	FD CAN (TCAN1042HGV) :
		PB7 - STB (digital output) - TCAN standby pin;
		PB8-9 - RX and TX respectively (FDCAN1, alternate function 9);

	User SPI:
		PC10-PC12 - SCK, MISO, MOSI respectively (SPI3, alternate function 6);
		PD2 - CS (digital output) - user SPI chip select;

	USB:
		PA11-PA12 - DM and DP respectively (alternate function ?? idk if it even an alternate function);

	EEPROM:
		PA8-PA9 - SDA and SCL respectively (I2C2, alternate function 4);

	Motor driver (DRV8701P):
		PC8-PC9 - IN1 and IN2 respectively (TIM8 PWM CH3-4, alternate function 4) - driver control pins;
		PB13 - nFault (digital input, external interrupt) - pulled logic low with fault condition;
		PB14 - SNSOUT (digital input, external interrupt) - pulled logic low when the drive current hits the current chopping threshold;
		PB15 - nSleep (digital output) - Pull logic low to put device into a low-power sleep mode with FETs High-Z;
		PB2 - SO (ADC2 IN12) - Voltage on this pin is equal to the SP voltage times AV plus an offset;

	Motor encoder:
		PC6-7 - encoder input 1 and 2 respectively (TIM3 encoder mode Ch1-2, alternate function 2);

	USB-UART (CP2102N-A01-GQFN20R):
		PB10-11 - TX and RX respectively (UART3, alternate function 7);
		PB12 - wakeup  (input or output ??) -  Remote USB wakeup interrupt input:

	RS-485 (THDV1450):
		PC4-5 - TX and RX respectively (UART1, alternate function 7);
		PB0 - RW (digital output) - control recieve and transmit mode of the device. "1" - transiver, "0" - receiver;

	BMS interface:
		PA2-3 - TX and RX respectively (UART2, alternate function 7);
		PA4 - wakeup (digital output) - ... wakes up BMS;
		PA5 - nFault (digital input, external interrupt) -  Pulled logic low with fault condition;

	User buttons:
		PC13-PC15 - button 1 to 3 respectively (digital input);

	User potentiometers:
		PC1-3 - potentiometers 1 to 3 respectively (ADC1 IN7-9 respectively)

	Current sensor (ACS711KEXLT-15AB-T):
		PA0 - VIOut (ADC1 IN1) - Voltage proportional to the flowing current;

	Current sensor (INA240A1PW):
		PA1 - OUT (ADC1 IN2) - Voltage proportional to the flowing current;

	TIM15 - counts time with 0.1 ms precision for mistakes log

	TIM16 - timer for proper delay implementation

	TIM17 - high speed counter for precise speed calculations.

 */

// Обязательно протестировать !!
/*
	@brief Makes log entry with given mistake code at current time.
		If mistake code is equal to 0 doens't log it

	@param[in] mistake_code - mistakes code to be written to log.

	@return Mistake code that was inputted to the function
 */
uint32_t add_to_mistakes_log(uint32_t mistake_code)
{
	if(mistake_code == 0)
	{
		return 0;
	}

	mistakes_log[mistakes_log_pointer].mistake_code = mistake_code;
	mistakes_log[mistakes_log_pointer].mistake_time_in_seconds = time_from_log_enable_in_seconds;
	mistakes_log[mistakes_log_pointer].mistake_time_in_milliseconds = TIM15->CNT;

	++mistakes_log_pointer;
	if (mistakes_log_pointer == MISTAKES_LOG_SIZE)
	{
		mistakes_log_pointer = 0;
	}

	return mistake_code;
}




/*!
	@brief 	sets up SYSCLK to SYSCLK_FREQUENCY with taking into account problems with different sources

	Tries to start up HSE. Calls PLL startup functions with HSE as a source if it started correctly and HSI otherwise.

	@return	mistake code. If HSE fails and PLL starts - 3, if PLL fails with any source - PLL fail code.

	@Documentation:
	> STM32G4 reference manual chapter 5 (Flash for category 2 devices) - flash access latency (5.3.3);
	> STM32G4 reference manual chapter 7 (RCC) - all information about clock setup.
 */


// *** System clock prescalers *** //
#if (SYSCLK_FREQUENCY >= 32000000)
	#define	PLLN_VALUE  			(SYSCLK_FREQUENCY / 2000000)
	#define PLLR_VALUE				(0U) // PLLR is dividing by 2
#endif

#if (SYSCLK_FREQUENCY < 32000000)
	#define	PLLN_VALUE  			(SYSCLK_FREQUENCY / 500000)
	#define PLLR_VALUE				(3U) // PLLR is dividing by 8
#endif

#define PLLM_VALUE 				(1U) // PLLM is dividing by 2
#define PLLM_VALUE_WITH_HSI		(3U) // PLLM is dividing by 4
#define PLLQ_VALUE				(3U) // PLLQ is dividing by 8
#define PLLP_VALUE 				(8U) // PLLP current divider value

// Над этой функций надо еще поработать, улучшить коментарии и проверить до конца логику работы, а также точно определеиться как где и какие ошибки должны обрабатываться

// Кроме этго в документации рекуомендуется разгонять и замедлять часы при большом шаге поэтапно. Если происходит разгон больше чем за 80 MHz, стоит сделать промежуточный шаг длительностью 1микросекунду на редней частоте.
uint32_t system_clock_setup(void)
{

	/* Flash read access latency from SYSCLK_FREQUENCY. See RM chapter 3.3.4 */

	#if ( SYSCLK_FREQUENCY > 120000000 )
		FLASH->ACR &= ~ FLASH_ACR_LATENCY_Msk;					// Clear latency setup
		FLASH->ACR |= FLASH_ACR_LATENCY_4WS;	// 0x04 - 5 CPU cycles read latency (4 + 1)

	#elif ( SYSCLK_FREQUENCY > 90000000 )
		FLASH->ACR &= ~ FLASH_ACR_LATENCY_Msk;					// Clear latency setup
		FLASH->ACR |= FLASH_ACR_LATENCY_3WS;	// 0x03 - 4 CPU cycles read latency (3 + 1)

	#elif ( SYSCLK_FREQUENCY > 60000000 )
		FLASH->ACR &= ~ FLASH_ACR_LATENCY_Msk;					// Clear latency setup
		FLASH->ACR |= FLASH_ACR_LATENCY_2WS;	// 0x02 - 3 CPU cycles read latency (2 + 1)

	#elif ( SYSCLK_FREQUENCY > 30000000 )
		FLASH->ACR &= ~ FLASH_ACR_LATENCY_Msk;					// Clear latency setup
		FLASH->ACR |= FLASH_ACR_LATENCY_1WS;	// 0x01 - 2 CPU cycles read latency (1 + 1)

	#else
		FLASH->ACR &= ~ FLASH_ACR_LATENCY_Msk;					// Clear latency setup - 1 CPU cycle read latency
	#endif

	RCC->CR |= RCC_CR_HSEON;

	for ( uint32_t i = 0; i < DUMMY_DELAY_VALUE; ++i )
	{
		if ( (RCC->CR & RCC_CR_HSERDY) == RCC_CR_HSERDY )
		{
			// Enable clock security system
			RCC->CR |= RCC_CR_CSSON;

			const uint32_t pll_setup_return_value = pll_setup(HSE_IS_OK);

			if ( pll_setup_return_value == 0 )
			{
				// Everything is ok. HSI will be stopped for so it won't consume energy
				RCC->CR &= ~RCC_CR_HSION_Msk;
				return 0;
			}
			else
			{
				RCC->CR &= ~RCC_CR_HSEON;				// Stop HSE so it won't consume power.
				FLASH->ACR &= ~FLASH_ACR_LATENCY_Msk;	// Reset FLASH latency because HSI is 16 Mhz
				return PLL_FAILED;						// PLL fails, so HSI is a clock source
			}
		}
	}

	// HSE hasn't started
	RCC->CR &= ~RCC_CR_HSEON;	// Stop HSE so it won't consume power.
	add_to_mistakes_log(HSE_FAILED_TO_START);

	const uint32_t pll_setup_return_value = pll_setup(HSE_IS_NOT_OK);

	if ( pll_setup_return_value )
	{
		return pll_setup_return_value;
	}

	return 3; // HSE fail code
}

/*!
	@brief	starts PLL. Takes into account HSE status

	@param[in] is_HSE_clock_source - status of HSE. If HSE is ok uses it as PLL clock source, uses HSI otherwise.

	@return		Mistake code: 0 - if everything s ok, 1 - if PLL start fail, 2 - if PLL fails to start as a clock source

	@limitations 	For the sake of simplifying usage of the code only one multiplier of the frequency calculation equation should be variable.
					Only frequencies multiple to 2Mhz and starting from 16Mhz are allowed: 16, 18, 20 ... 62, 64Mhz. Because values of PLLN lower then 8 and higher then 43 are not allowed.
					This limitation allows switching frequency source between PLL on HSI and HSE without troubles.

	@Documentation:
		> STM32G0x1 reference manual chapter 5 (RCC) - all information about clock setup.

	@Calculations
	> When using 8 Mhz HSE (external oscillator) as a clock sours for PLL (default setup).

		We take:

		PLLR = 4
		PLLM = 1
		PLLN = SYSCLK_FREQUENCY / 2000000

					oscillator_frequensy * PLLN       8 * PLLN
		SYSCLK =  ------------------------------- = ------------
						   PLLM * PLLR				   1 * 4

		PLLN can be changed from 8 to 86, but only 8 to 43 are allowed, so we can have any frequency from 16 to 64 Mhz with a step of 2 Mhz and change in only one variable

	> When using 16 Mhz HSI (internal oscillator) as a clock sours for PLL (critical - when HSE can't be enabled)

		We take:

		PLLR = 4
		PLLM = 2
		PLLN = SYSCLK_FREQUENCY / 2000000

					oscillator_frequensy * PLLN       16 * PLLN
		SYSCLK =  ------------------------------- = ------------
						   PLLM * PLLR				   2 * 4

		PLLN can be changed from 8 to 86, but only 8 to 43 are allowed, so we can have any frequency from 16 to 64 Mhz with a step of 2 Mhz and change in only one variable

	@note 	If PLL is not working (i don't know if it is really possible) HSI is used as a clock source no matter of HSE status.

	@note 	AHB prescaler always equals to 1, so HCLK = SYSCLK

	@note 	APB prescaler always equals to 1
 */
uint32_t pll_setup(uint32_t is_HSE_clock_source)
{
	RCC->PLLCFGR = 0;

	if ( is_HSE_clock_source == HSE_IS_OK )
	{
		RCC->PLLCFGR |= PLLP_VALUE << RCC_PLLCFGR_PLLPDIV_Pos
				| PLLR_VALUE << RCC_PLLCFGR_PLLR_Pos
				| RCC_PLLCFGR_PLLREN
				| PLLQ_VALUE << RCC_PLLCFGR_PLLQ_Pos
				| PLLN_VALUE << RCC_PLLCFGR_PLLN_Pos
				| PLLM_VALUE << RCC_PLLCFGR_PLLM_Pos
				| RCC_PLLCFGR_PLLSRC_HSE;
	}
	else
	{
		RCC->PLLCFGR |= PLLP_VALUE << RCC_PLLCFGR_PLLPDIV_Pos
				| PLLR_VALUE << RCC_PLLCFGR_PLLR_Pos
				| RCC_PLLCFGR_PLLREN
				| PLLQ_VALUE << RCC_PLLCFGR_PLLQ_Pos
				| PLLN_VALUE << RCC_PLLCFGR_PLLN_Pos
				| PLLM_VALUE_WITH_HSI << RCC_PLLCFGR_PLLM_Pos
				| RCC_PLLCFGR_PLLSRC_HSI;
	}

	RCC->CR |= RCC_CR_PLLON;

	uint32_t safety_delay_counter = 0;

	while ( (RCC->CR & RCC_CR_PLLRDY) != RCC_CR_PLLRDY )
	{
		++safety_delay_counter;
		if ( safety_delay_counter > DUMMY_DELAY_VALUE )
		{
			RCC->CR &= ~RCC_CR_PLLON;	// Stop PLL so it won't consume power.
			add_to_mistakes_log(PLL_FAILED);
			return 1;					// PLL startup fail code
		}
	}

	// At that point PLL is on and we can use it as a SYSCLK source
	RCC->CFGR &= ~RCC_CFGR_SW_Msk;
	RCC->CFGR |= RCC_CFGR_SW_PLL;

	safety_delay_counter = 0;

	while ( (RCC->CFGR & RCC_CFGR_SWS_PLL) != RCC_CFGR_SWS_PLL )
	{
		++safety_delay_counter;
		if ( safety_delay_counter > DUMMY_DELAY_VALUE )
		{
			FLASH->ACR &= ~FLASH_ACR_LATENCY_Msk;	// Reset FLASH latency because HSI is 16 Mhz

			// Use HSI as a clock source
			RCC->CFGR &= ~RCC_CFGR_SW_Msk;
			RCC->CFGR |= RCC_CFGR_SW_HSE ;

			add_to_mistakes_log(PLL_FAILED);
			return 1;								// PLL as clock source startup fail code
		}
	}

	return 0;	// Everything is fine and online
}

/*
	@brief	Non-maskable interrupt handler. Called when HSE fails. Tries to start PLL with HSI as a source

	@Documentation:
		> STM32G0x1 reference manual chapter 5 (RCC) - clock security system (5.2.8).

	Right now I don't know if this interrupt can be called by any other system.
	Because of it it doesn't check for call reason but can be changed in the future.
 */
void NMI_Handler()
{
	// Clear the clock security system interrupt flag
	RCC->CICR |= RCC_CICR_CSSC;

	add_to_mistakes_log(HSE_FAILED_WHILE_RUNNING);

	// Wait until PLL is fully stopped
	while ( (RCC->CR & RCC_CR_PLLRDY) == RCC_CR_PLLRDY ){}

	// Isn't trying to use HSA, uses HSI instead. Tries to start PLL.
	const uint32_t pll_setup_return_value = pll_setup(HSE_IS_NOT_OK);

	if (pll_setup_return_value)
	{
		add_to_mistakes_log(PLL_FAILED);
	}
}

/*
	@brief	Sets up GPIO for all needed on device functions

	@Documentation
		> STM32G0x1 reference manual chapter 5 (RCC) - all information about peripheral locations and enabling.
		> STM32G0x1 reference manual chapter 6 (GPIO) - information about GPIO setup.
		> STM32G071x8/xb datasheet chapter 4 (Pinouts, pin description and alternate functions) - information about alternate function on the pins.

	@note	In stm32g0 all GPIO are in analog mode by default (0b11)
 */
void gpio_setup(void)
{
	// *** GPIO peripheral clock enable *** //
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIODEN;

	// *** Port A full GPIO setup *** //
	GPIOA->MODER &= ~((GPIO_MODER_MSK << GPIO_MODER_MODE0_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE1_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE2_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE3_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE4_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE5_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE8_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE9_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE11_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE12_Pos));

	GPIOA->MODER |= (GPIO_ANALOG_IN << GPIO_MODER_MODE0_Pos)
			| (GPIO_ANALOG_IN << GPIO_MODER_MODE1_Pos)
			| (GPIO_ALTERNATE << GPIO_MODER_MODE2_Pos)
			| (GPIO_ALTERNATE << GPIO_MODER_MODE3_Pos)
			| (GPIO_DIGITAL_OUT << GPIO_MODER_MODE4_Pos)
			| (GPIO_DIGITAL_IN << GPIO_MODER_MODE5_Pos)
			| (GPIO_ALTERNATE << GPIO_MODER_MODE8_Pos)
			| (GPIO_ALTERNATE << GPIO_MODER_MODE9_Pos)
			| (GPIO_DIGITAL_OUT << GPIO_MODER_MODE11_Pos)
			| (GPIO_DIGITAL_OUT << GPIO_MODER_MODE12_Pos);

	GPIOA->OSPEEDR |= (GPIO_OSPEED_LOW << GPIO_OSPEEDR_OSPEED2_Pos)
			| (GPIO_OSPEED_LOW << GPIO_OSPEEDR_OSPEED3_Pos)
			| (GPIO_OSPEED_LOW << GPIO_OSPEEDR_OSPEED11_Pos)
			| (GPIO_OSPEED_LOW << GPIO_OSPEEDR_OSPEED12_Pos);	// LOW speed (up to 50 MHz) for UART and USB

	GPIOA->AFR[0] |= (ALTERNATE_FUNCTION_7 << GPIO_AFRL_AFSEL2_Pos)
			| (ALTERNATE_FUNCTION_7 << GPIO_AFRL_AFSEL3_Pos);	// USART2 setup

	GPIOA->AFR[1] |= (ALTERNATE_FUNCTION_4 << GPIO_AFRH_AFSEL8_Pos)
			| (ALTERNATE_FUNCTION_4 << GPIO_AFRH_AFSEL9_Pos);	// I2C2 setup

	// *** Port B full GPIO setup *** //
	GPIOB->MODER &= ~((GPIO_MODER_MSK << GPIO_MODER_MODE0_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE2_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE3_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE4_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE5_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE6_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE7_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE8_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE9_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE10_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE11_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE12_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE13_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE14_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE15_Pos));

	GPIOB->MODER |= (GPIO_DIGITAL_OUT << GPIO_MODER_MODE0_Pos)
			| (GPIO_ANALOG_IN << GPIO_MODER_MODE2_Pos)
			| (GPIO_DIGITAL_OUT << GPIO_MODER_MODE3_Pos)
			| (GPIO_DIGITAL_OUT << GPIO_MODER_MODE4_Pos)
			| (GPIO_DIGITAL_OUT << GPIO_MODER_MODE5_Pos)
			| (GPIO_DIGITAL_OUT << GPIO_MODER_MODE6_Pos)
			| (GPIO_DIGITAL_OUT << GPIO_MODER_MODE7_Pos)
			| (GPIO_ALTERNATE << GPIO_MODER_MODE8_Pos)
			| (GPIO_ALTERNATE << GPIO_MODER_MODE9_Pos)
			| (GPIO_ALTERNATE << GPIO_MODER_MODE10_Pos)
			| (GPIO_ALTERNATE << GPIO_MODER_MODE11_Pos)
			| (GPIO_DIGITAL_IN << GPIO_MODER_MODE12_Pos)
			| (GPIO_DIGITAL_IN << GPIO_MODER_MODE13_Pos)
			| (GPIO_DIGITAL_IN << GPIO_MODER_MODE14_Pos)
			| (GPIO_DIGITAL_OUT << GPIO_MODER_MODE15_Pos);

	GPIOB->OSPEEDR |= (GPIO_OSPEED_LOW << GPIO_OSPEEDR_OSPEED8_Pos)
			| (GPIO_OSPEED_LOW << GPIO_OSPEEDR_OSPEED9_Pos)
			| (GPIO_OSPEED_LOW << GPIO_OSPEEDR_OSPEED10_Pos)
			| (GPIO_OSPEED_LOW << GPIO_OSPEEDR_OSPEED11_Pos);	// LOW speed (up to 50 Mhz) for USART3 and FDCAN1.

	GPIOB->AFR[1] |= (ALTERNATE_FUNCTION_9 << GPIO_AFRH_AFSEL8_Pos)
			| (ALTERNATE_FUNCTION_9 << GPIO_AFRH_AFSEL9_Pos)
			| (ALTERNATE_FUNCTION_7 << GPIO_AFRH_AFSEL10_Pos)
			| (ALTERNATE_FUNCTION_7 << GPIO_AFRH_AFSEL11_Pos);	// FDCAN1 and USART3 setup.

	// *** Port C full GPIO setup *** //
	GPIOC->MODER &= ~((GPIO_MODER_MSK << GPIO_MODER_MODE1_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE2_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE3_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE4_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE5_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE6_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE7_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE8_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE9_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE10_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE11_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE12_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE13_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE14_Pos)
			| (GPIO_MODER_MSK << GPIO_MODER_MODE15_Pos));

	GPIOC->MODER |= (GPIO_ANALOG_IN << GPIO_MODER_MODE1_Pos)
			| (GPIO_ANALOG_IN << GPIO_MODER_MODE2_Pos)
			| (GPIO_ANALOG_IN << GPIO_MODER_MODE3_Pos)
			| (GPIO_ALTERNATE << GPIO_MODER_MODE4_Pos)
			| (GPIO_ALTERNATE << GPIO_MODER_MODE5_Pos)
			| (GPIO_ALTERNATE << GPIO_MODER_MODE6_Pos)
			| (GPIO_ALTERNATE << GPIO_MODER_MODE7_Pos)
			| (GPIO_ALTERNATE << GPIO_MODER_MODE8_Pos)
			| (GPIO_ALTERNATE << GPIO_MODER_MODE9_Pos)
			| (GPIO_ALTERNATE << GPIO_MODER_MODE10_Pos)
			| (GPIO_ALTERNATE << GPIO_MODER_MODE11_Pos)
			| (GPIO_ALTERNATE << GPIO_MODER_MODE12_Pos)
			| (GPIO_DIGITAL_IN << GPIO_MODER_MODE13_Pos)
			| (GPIO_DIGITAL_IN << GPIO_MODER_MODE14_Pos)
			| (GPIO_DIGITAL_IN << GPIO_MODER_MODE15_Pos);

	GPIOC->OSPEEDR |= (GPIO_OSPEED_LOW << GPIO_OSPEEDR_OSPEED4_Pos)
			| (GPIO_OSPEED_LOW << GPIO_OSPEEDR_OSPEED5_Pos)
			| (GPIO_OSPEED_LOW << GPIO_OSPEEDR_OSPEED8_Pos)
			| (GPIO_OSPEED_LOW << GPIO_OSPEEDR_OSPEED9_Pos)
			| (GPIO_OSPEED_LOW << GPIO_OSPEEDR_OSPEED10_Pos)
			| (GPIO_OSPEED_LOW << GPIO_OSPEEDR_OSPEED11_Pos)
			| (GPIO_OSPEED_LOW << GPIO_OSPEEDR_OSPEED12_Pos)
			| (GPIO_OSPEED_LOW << GPIO_OSPEEDR_OSPEED9_Pos);	// LOW speed (up to 50 Mhz) for USART1, TIM8 CH3-4 and SPI3.

	GPIOC->AFR[0] |= (ALTERNATE_FUNCTION_7 << GPIO_AFRL_AFSEL4_Pos)
			| (ALTERNATE_FUNCTION_7 << GPIO_AFRL_AFSEL5_Pos)
			| (ALTERNATE_FUNCTION_2 << GPIO_AFRL_AFSEL6_Pos)
			| (ALTERNATE_FUNCTION_2 << GPIO_AFRL_AFSEL7_Pos);	// USART1 and TIM3 CH1-2 setup.

	GPIOC->AFR[1] |= (ALTERNATE_FUNCTION_4 << GPIO_AFRH_AFSEL8_Pos)
			| (ALTERNATE_FUNCTION_4 << GPIO_AFRH_AFSEL9_Pos)
			| (ALTERNATE_FUNCTION_6 << GPIO_AFRH_AFSEL10_Pos)
			| (ALTERNATE_FUNCTION_6 << GPIO_AFRH_AFSEL11_Pos)
			| (ALTERNATE_FUNCTION_6 << GPIO_AFRH_AFSEL12_Pos);	// TIM8 CH3-4 and SPI3 setup.

	// *** Port D full GPIO setup *** //
	GPIOD->MODER &= ~((GPIO_MODER_MSK << GPIO_MODER_MODE2_Pos));

	GPIOD->MODER |= (GPIO_DIGITAL_OUT << GPIO_MODER_MODE2_Pos);
}

/*!
	@brief	Sets up all used on the board timers and enables desired interrupts

	@Documentation:
		> STM32G0x1 reference manual chapter 5 (RCC) - all information about peripheral locations and enabling;
		> STM32G0x1 reference manual chapter 20 (TIM1) - TIM1 setup information;
		> STM32G0x1 reference manual chapter 21 (TIM2/TIM3) - TIM2 and TIM3 setup information;
		> STM32G0x1 reference manual chapter 23 (TIM14) - TIM14 setup information;
	 	> Cortex-M0+ programming manual for stm32 chapter 4 - SysTick timer (STK)(4.4).

	Timer channels allocations with respect to function.

	Timer 			| function
	---------------	| ----------------------------------------------------
	TIM1			| generates PWM for motor control on all 4 channels
	TIM1 CH1		| DRV8848 AIN1
	TIM1 CH2 		| DRV8848 AIN2
	TIM1 CH3		| DRV8848 BIN2
	TIM1 CH4		| DRV8848 BIN1
	--------------- | ----------------------------------------------------
	TIM2			| Motor1 encoder pulses counter
	TIM2 CH1		| Motor1 encoder phase A counter
	TIM2 CH2		| Motor1 encoder phase B counter
	--------------- | ----------------------------------------------------
	TIM3			| Motor2 encoder pulses counter
	TIM3 CH1		| Motor2 encoder phase A counter
	TIM3 CH2		| Motor2 encoder phase B counter
	--------------- | ----------------------------------------------------
	TIM14			| Counts time in 0.125 millisecond steps for debug log.
					| Resets exactly every minute
					|

	@calculation

	TIM14_prescaler = -----------------
 */
void timers_setup(void)
{
	//*** Timers peripheral clock enable ***//
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3EN;
	RCC->APB2ENR |= RCC_APB2ENR_TIM8EN | RCC_APB2ENR_TIM15EN | RCC_APB2ENR_TIM16EN | RCC_APB2ENR_TIM17EN;

	//*** TIM8 PWM setup ***//
	TIM8->PSC = 0; 				// Timer speed = bus speed
	TIM8->ARR = PWM_PRECISION;
	TIM8->CCMR2 |= 0x6868;	// PWM mode 1 enable and output compare preload enable on channels 3 and 4
	TIM8->CCER |= 0x1100;	// Enable CH1-4
	TIM8->CR1 |= TIM_CR1_ARPE;	// Enable Auto reload preload
	TIM8->BDTR |= TIM_BDTR_MOE;	// Main output enable
	TIM8->CR1 |= TIM_CR1_CEN; // Enable timer
	TIM8->CCR3 = 0;
	TIM8->CCR4 = 0;

	//*** Timer3 encoder setup ***//
	TIM3->ARR = 65535; 		// 2^16-1 - maximum value for this timer. No prescaler, so timer is working with max speed
	TIM3->CCER |= 0x02;		// Should be uncommented if encoder direction reversal is needed
	TIM3->SMCR |= 0x03;		// Encoder mode setup
	TIM3->CNT = 0;			// Clear counter before start
	TIM3->CR1 |= TIM_CR1_CEN;

	//*** TIM15 setup ***//
	TIM15->PSC |= (uint32_t)(SYSCLK_FREQUENCY / 36000 - 1); //
	TIM15->ARR = 35999; 	// 60 second * 60 millisecond * 10 - 1 to get 0.1 milliseconds step
	TIM15->CNT = 0;			// Clear counter before start
	TIM15->DIER |= TIM_DIER_UIE;
	NVIC_EnableIRQ(TIM1_BRK_TIM15_IRQn);
	TIM15->CR1 |= TIM_CR1_CEN;

	//*** TIM16 setup ***//
	TIM16->PSC |= (uint32_t)(SYSCLK_FREQUENCY / 1000 - 1); 	// One millisecond step
	TIM16->CNT = 0;			// Clear counter before start
	TIM16->DIER |= TIM_DIER_UIE;
	NVIC_EnableIRQ(TIM1_UP_TIM16_IRQn);
	TIM16->CR1 |= TIM_CR1_OPM;	// One pulse mode. Counter don't need to be started

//	//For precise speed measurement (
//	//*** TIM17 setup ***//
//	// Used to count millisecond for speed calculations
//	TIM17->PSC |= 47; 	// 24 Mhz clock -> 500.000 pulses per second into CNT // Need to be changed to automaticly calculate value depending on the encoder PPR and system frequensy
//	TIM17->ARR = 65535; 	// 2^16-1 - maximum value for this timer, so program should have minimum 16 speed calculations per second not to loose data
//	TIM17->CNT = 0;			// Clear counter before start
//	TIM17->CR1 |= TIM_CR1_CEN;

//	//*** System timer setup ***//
//	SysTick->LOAD = SYSCLK_FREQUENCY / (8 * SYSTICK_FREQUENCY) - 1;
//	SysTick->VAL = 0;
//	NVIC_EnableIRQ(SysTick_IRQn);
//	SysTick->CTRL |= 0x03; // Start SysTick, enable interrupt
}

/*
	@brief Sets up ADC
 */
void adc_2_setup(uint16_t * array_to_write_to)
{
	/*
		Перечень используемых АЦП:

		ADC2_IN7 - потенциометр №1
		ADC2_IN8 - потенциометр №2
		ADC2_IN9 - потенциометр №3

		ADC2_IN1 - ACS711 измерение тока
		ADC2_IN2 - INA240A измерение тока

		ADC2_IN12 - DRV измерение тока
	 */

	// Set ADC clock
	RCC->CCIPR |= (2U) << RCC_CCIPR_ADC12SEL_Pos; // SYSCLK as a clock source for ADC1-2.So ADC is working in asynchronous mode.

	// Enable ADC clocking
	RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN;

	// Get ADC from deep power down mode
	ADC2->CR &= ~(ADC_CR_DEEPPWD);

	// Start up internal voltage regulator
	ADC2->CR |= ADC_CR_ADVREGEN;
	delay_in_milliseconds(2); // wait until Vreg is on. Должно быть 20us по даташиту, поэтому задержка будет слишком длинной, надо будет исправить со временем.

	// ADC calibration
	ADC2->CR |= ADC_CR_ADCAL;
	while(ADC2->CR & ADC_CR_ADCAL){} // Wait until calibration is complite

	// Enable ADC
	ADC2->ISR |= ADC_ISR_ADRDY; // clear ready flag
	ADC2->CR |= ADC_CR_ADEN;
	while(!(ADC2->ISR & ADC_ISR_ADRDY)){} // Wait until adc is ready

	// All channels will be sampled with 12.5 ADC clock cycles
	ADC2->SMPR1 |=  2 << ADC_SMPR1_SMP9_Pos | 2 << ADC_SMPR1_SMP8_Pos | 2 << ADC_SMPR1_SMP7_Pos | 2 << ADC_SMPR1_SMP2_Pos |  2 << ADC_SMPR1_SMP1_Pos;
	ADC2->SMPR2 |=  2 << ADC_SMPR2_SMP12_Pos;

	// First setup to read only data from potentiometers
	ADC2->SQR1 |= 9 << ADC_SQR1_SQ3_Pos | 8 << ADC_SQR1_SQ2_Pos | 7 << ADC_SQR1_SQ1_Pos | 2 << ADC_SQR1_L_Pos; // 3 conversions

//	// Second setup, to read data from all 6 sources
//	ADC2->SQR1 |= 1 << ADC_SQR1_SQ4_Pos | 9 << ADC_SQR1_SQ3_Pos | 8 << ADC_SQR1_SQ2_Pos | 7 << ADC_SQR1_SQ1_Pos | 5 << ADC_SQR1_L_Pos; // 6 conversions
//	ADC2->SQR2 |= 12 << ADC_SQR2_SQ6_Pos | 2 << ADC_SQR2_SQ5_Pos;

//	// slow down
//	ADC12_COMMON->CCR |= 4 << ADC_CCR_PRESC_Pos;

	// enable DMA
	ADC2->CFGR |= ADC_CFGR_DMAEN | ADC_CFGR_DMACFG;
	adc_dma_setup(array_to_write_to);

}


/*
	@brief Sets up DMA
 */
void adc_dma_setup(uint16_t * array_to_write_to)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMAMUX1EN;	// Enable DMA


	// DMA channel 1 setup
	DMA1_Channel1->CPAR = (uint16_t *)&(ADC2->DR);	// Direct read from TIM15->CNT regester
	DMA1_Channel1->CMAR = (uint16_t *)array_to_write_to;	// Memory address to write to. Уже указатель, поэтому нет необходимости получать его адрес

	// for 3 conversions
	DMA1_Channel1->CNDTR = 3;			// Number of transfers
	DMA1_Channel1->CCR |= 1 << DMA_CCR_MSIZE_Pos | 1 << DMA_CCR_PSIZE_Pos | DMA_CCR_MINC | DMA_CCR_CIRC;		// 16 bit in and out, circular mode, increment in memmory
	DMAMUX1_Channel0->CCR |= (36 << DMAMUX_CxCR_DMAREQ_ID_Pos); 	// ADC2 is 36
	DMA1_Channel1->CCR |= DMA_CCR_EN;		// enable DMA

//	// for 6 conversions
//	DMA1_Channel1->CNDTR = 6;			// Number of transfers
//	DMA1_Channel1->CCR |= 1 << DMA_CCR_MSIZE_Pos | 1 << DMA_CCR_PSIZE_Pos | DMA_CCR_MINC | DMA_CCR_CIRC;		// 16 bit in and out, circular mode, increment in memmory
//	DMA1_Channel1->CCR |= DMA_CCR_EN;		// enable DMA

}

/*
	@brief Enable SPI3 transmission with respect to given SPI speed
 */
void basic_spi3_setup(uint32_t transmittion_speed_in_hz)
{
	// 10 Mhz is standard high SPI speed, so in general, we should not use higher speeds. If input speed is greater than 10Mhz set it to 5Mhz and write input mistake to log
	if(transmittion_speed_in_hz > 10000000)
	{
		add_to_mistakes_log(WRONG_SPI3_FREQUENCY_INPUT);
		transmittion_speed_in_hz = 5000000;
	}

	// Enable SPI clocking
	RCC->APB1ENR1 |= RCC_APB1ENR1_SPI3EN;


	// SPI SPI_CR1_BR value determination
	uint32_t baud_rate_devider = 2;
	uint32_t baud_rate = 0;

	for ( int i = 0; i < 8; ++i )
	{
		if ( (SYSCLK_FREQUENCY / baud_rate_devider) < transmittion_speed_in_hz )
		{
			break;
		}
		++baud_rate;
		baud_rate_devider *= 2;
	}

	// SPI setup
	SPI3->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI | (baud_rate << SPI_CR1_BR_Pos) | SPI_CR1_MSTR; // Equal to SPI3->CR1 |= 0x0314;
	SPI3->CR2 |= SPI_CR2_FRXTH;
	SPI3->CR1 |= SPI_CR1_SPE;
}

/*
	@brief Transmit and receive single byte with SPI 1
 */
uint8_t spi3_write_single_byte(const uint8_t byte_to_be_sent)
{
	uint32_t safety_delay_counter = 0;

	// Wait until transmit buffer is empty
	while((SPI3->SR & SPI_SR_TXE) != SPI_SR_TXE)
	{
		++safety_delay_counter;
		if ( safety_delay_counter > DUMMY_DELAY_VALUE )
		{
			add_to_mistakes_log(SPI3_TRANSMISSION_FAIL);
			return 0;
		}
	}

	// Write single byte into the Data Register with single byte access
	*((volatile uint8_t *)&SPI3->DR) = byte_to_be_sent;

	// Wait until answer will appear in RX buffer
	while(((SPI3->SR & SPI_SR_RXNE) != SPI_SR_RXNE)){}
//	while(((SPI->SR & 0x81) == 0x80)){}

	// Return value from RX buffer
	return SPI3->DR;
}





void basic_uart2_setup(const uint32_t transmission_speed_in_bauds)
{
	RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;

	USART2->BRR = SYSCLK_FREQUENCY/transmission_speed_in_bauds;
	USART2->CR1 |= USART_CR1_UE;
	USART2->CR1 |= USART_CR1_TE | USART_CR1_RE;
}

// @brief Sends given byte when TX buffer is empty
void uart2_send_byte(const uint8_t message_byte)
{
	while((USART2->ISR & USART_ISR_TC) != USART_ISR_TC){}
	while((USART2->ISR & USART_ISR_TXE_TXFNF) != USART_ISR_TXE_TXFNF) {}
	USART2->TDR = message_byte;
}


void TIM1_UP_TIM16_IRQHandler()
{
	TIM16->SR &= ~TIM_SR_UIF;
	delay_is_finished = yes;
}


void delay_in_milliseconds(const uint16_t time_in_millisecond){
	TIM16->ARR = time_in_millisecond-1;
	TIM16->CR1 |= TIM_CR1_CEN;
	delay_is_finished = no;
	while(delay_is_finished == no){}

}


void full_device_setup(uint32_t should_inclued_interfaces, uint32_t should_setup_interrupts)
{

	add_to_mistakes_log(system_clock_setup());

	gpio_setup();

	timers_setup();

//	if (should_setup_interrupts == yes)
//	{
//		interrupts_setup();
//	}
//
//	if(should_inclued_interfaces == yes)
//	{
//		intrfaces_setup();
//	}

	return;
}

// Обязательно протестировать!!
/*
	@brief TIM14 overflow interrupt handler - counts minutes from enable of mistakes log.

	@note If device have EEprom - saves mistake log into it every minute.
 */
void TIM1_BRK_TIM15_IRQHandler()
{
	TIM15->SR &= ~TIM_SR_UIF;

	time_from_log_enable_in_seconds += 1;

	// Place to put code to write into EEPROM (for development of future devices)
}

