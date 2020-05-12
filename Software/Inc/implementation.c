#include "implementation.h"


// ******** MCU connections overview ******** //
/*
	LEDs:
		PB3-PB6 (digital outputs);

	FD CAN (TCAN1042HGV) :
		PB7 - STB (digital output) - TCAN standby pin;
		PB8-9 - RX and TX respectively (FDCAN1, alternate function 9);

	User SPI:
		PC10-PC12 - SCK, MISO, MOSI respectively (SPI3, alternate function 0);
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
		PA0 - VIOut (ADC1 IN1) - Voltage proportional to thw flowing current;

	Current sensor (INA240A1PW):
		PA1 - OUT (ADC1 IN2) - Voltage proportional to the flowing current;

	TIM15 - high speed counter for precise speed calculations.

	TIM16 - timer for proper delay implementation

	TIM17 - counts time with 0.1 ms precision for mistakes log

 */


