//#if !defined(__SOFT_FP__) && defined(__ARM_FP)
//  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
//#endif

#define VAR_DECLS // Global variables initialization

#include "device.h"

void dummy_delay(uint32_t delay_time);

uint32_t system_counter = 0;

	// Бесит, похооду, чтобы запустить хоть что-то с CAN нужно будет прописывать собтсвенные адреса RAM, в которые я буду писать

#define FDCAN_11_BIT_FILTER_Pos 				(SRAMCAN_BASE + 0x0000) // FLSSA
#define FDCAN_11_BIT_FILTER_RECORD_SIZE 		(4U) // 4 bytes = 1 word, max number of elements = 28

#define FDCAN_29_BIT_FILTER_Pos					(SRAMCAN_BASE + 0x0070) // FLESA
#define FDCAN_29_BIT_FILTER_RECORD_SIZE 		(8U) // 8 bytes = 2 words, max number of elements = 8

#define FDCAN_RX_FIFO_0_Pos						(SRAMCAN_BASE + 0x00B0) // F0SA
#define FDCAN_RX_FIFO_0_RECORD_SIZE				(72U) // 72 bytes = 18 word, max number of elements = 3

#define FDCAN_RX_FIFO_1_Pos						(SRAMCAN_BASE + 0x0188) // F1SA
#define FDCAN_RX_FIFO_1_RECORD_SIZE				(72U) // 72 bytes = 18 word, max number of elements = 3

#define FDCAN_TX_EVENT_FIFO						(SRAMCAN_BASE + 0x0260) // EFSA
#define FDCAN_TX_EVENT_FIFO_RECORD_SIZE			(8U) // 8 bytes = 2 words, , max number of elements = 3

#define FDCAN_TX_BUFFER_Pos						(SRAMCAN_BASE + 0x0278) // TBSA
#define FDCAN_TX_BUFFER_RECORD_SIZE				(72U) // 72 bytes = 18 words, , max number of elements = 3


// FDCAN 11 bit filter defines
#define FDCAN_S0_SFT_Pos						(30U)
#define FDCAN_S0_SFEC_Pos						(27U)
#define FDCAN_S0_SFID1_Pos						(16U)
#define FDCAN_S0_SFID2_Pos						(0U)


// FDCAN RX buffer defines

#define FDCAN_T0_ESI_Pos 						(31U)
#define FDCAN_T0_XTD_Pos						(30U)
#define FDCAN_T0_RTR_Pos						(29U)
#define FDCAN_T0_11_bit_ID_Pos					(18U)
#define FDCAN_T0_29_bit_ID_Pos					(0U)

#define FDCAN_T1_MM_Pos							(24U)
#define FDCAN_T1_EFC_Pos						(23U)
#define FDCAN_T1_FDF_Pos						(21U)
#define FDCAN_T1_BRS_Pos						(20U)
#define FDCAN_T1_DLC_Pos						(16U)


void write_can_fd_simple_message();

uint32_t data_from_can = 0;
uint32_t data_position = 0;

int main(void)
{

	full_device_setup(no, no);

	// *** Данный пример разрабатывается именно для 48 Mhz частоты процессора и необходим для элементарного запуска CAN.
	// Соответственно перед реальным применением его нужно будет значительно переделать, улучшая слабывае моменты, которых достаточно *** //

	// CAN clock setup
	RCC->CCIPR |= (2U) << RCC_CCIPR_FDCANSEL_Pos; // PLLCLK is used as FDCAN clock source

	// Enable clocking of FDCAN
	RCC->APB1ENR1 |= RCC_APB1ENR1_FDCANEN;

	// Enable CAN setup update
	FDCAN1->CCCR |= FDCAN_CCCR_INIT;
	FDCAN1->CCCR |= FDCAN_CCCR_CCE;

	// FDCAN bit  timing setup
	// Clock source is 48 Mhz, I wont 2Mhz fast speed and 0.5 Mhz normal speed. Также в документации ошибка, если я все правильно понимаю ,
	// и там в примере с расчетом скорости передачи данных скорее всего ошибка и значение должно быть 0x00000A33: тогда все вверно, но надо будет проверить
	FDCAN1->DBTP = 0; // Reset register
	FDCAN1->DBTP = FDCAN_DBTP_TDC | 1 << FDCAN_DBTP_DBRP_Pos | 6 << FDCAN_DBTP_DTSEG1_Pos | 3 << FDCAN_DBTP_DTSEG2_Pos |  3 << FDCAN_DBTP_DSJW_Pos; // Prescaler of 2 so frequency is 24 tbs1 = 7 tbs2 = 4 actual data rate = 2Mbit/s

	FDCAN1->NBTP = 0; // Reset register
	FDCAN1->NBTP = 7 << FDCAN_NBTP_NBRP_Pos | 10 << FDCAN_NBTP_NTSEG1_Pos | 3 << FDCAN_NBTP_NTSEG2_Pos |  3 << FDCAN_NBTP_NSJW_Pos;	// Prescaler is 8 so frequency is 8Mhz, tbs1 = 11, tbs2 = 4 actual data rate is 0.5 Mbit/s

	// FDCAN mode setup. Test mode enabled with FDCAN and data rate switch enabled
	FDCAN1->CCCR |= FDCAN_CCCR_TEST | FDCAN_CCCR_FDOE | FDCAN_CCCR_BRSE; // Add | FDCAN_CCCR_MON to enable internal loop back mode

	// Enable loop back test mode
	FDCAN1->TEST |= FDCAN_TEST_LBCK;

	// Timestamp timer setup
	FDCAN1->TSCC |= 15 << FDCAN_TSCC_TCP_Pos | 1 << FDCAN_TSCC_TSS_Pos;


	// RX filter configuration
	// 4 11 bit different types of filetrs and 2 29 bit different types of filters
	FDCAN1->RXGFC |= 0 << FDCAN_RXGFC_LSE_Pos | 4 << FDCAN_RXGFC_LSS_Pos | 1 << FDCAN_RXGFC_ANFS_Pos | 1 << FDCAN_RXGFC_ANFE_Pos; // 0 - 29-bit filters, 4 11-bit filters, send not matching id to RX fifo 1
//	FDCAN1->XIDAM // i don't know what does it do


	// Наверное это не должно тут быть, но я все равно попробую это сюда положить
	uint32_t filters_11_bit[4] = {0, 0, 0 ,0};

	filters_11_bit[0] = 1 << FDCAN_S0_SFT_Pos | 1 << FDCAN_S0_SFEC_Pos | 168 << FDCAN_S0_SFID1_Pos | 168 << FDCAN_S0_SFID2_Pos;
	filters_11_bit[1] = 1 << FDCAN_S0_SFT_Pos | 1 << FDCAN_S0_SFEC_Pos | 238 << FDCAN_S0_SFID1_Pos | 238 << FDCAN_S0_SFID2_Pos;
	filters_11_bit[2] = 1 << FDCAN_S0_SFT_Pos | 1 << FDCAN_S0_SFEC_Pos | 367 << FDCAN_S0_SFID1_Pos | 367 << FDCAN_S0_SFID2_Pos;
	filters_11_bit[3] = 1 << FDCAN_S0_SFT_Pos | 1 << FDCAN_S0_SFEC_Pos | 543 << FDCAN_S0_SFID1_Pos | 543 << FDCAN_S0_SFID2_Pos;

	*(uint32_t *)(FDCAN_11_BIT_FILTER_Pos) =  filters_11_bit[0];
	*(uint32_t *)(FDCAN_11_BIT_FILTER_Pos + 4) =  filters_11_bit[1];
	*(uint32_t *)(FDCAN_11_BIT_FILTER_Pos + 8) =  filters_11_bit[2];
	*(uint32_t *)(FDCAN_11_BIT_FILTER_Pos + 12) =  filters_11_bit[3];


	// Finish CAN setup
	FDCAN1->CCCR &= ~ (FDCAN_CCCR_INIT | FDCAN_CCCR_CCE);

	// После этого в теории CAN должен быть инициализирован в тестовом режие, правда надо проверить в каком именно и готов к работе
	// сейчас надо попробовать записать в Rx fifo сообщеньку и прочитать ее в отладчике

	// Надо нсатроить маски


	// В теории маски настроены и можно попробовать отправлять данные

	uint32_t message_t0 = 0;
	message_t0 = 0 << FDCAN_T0_ESI_Pos | 0 << FDCAN_T0_XTD_Pos | 0 << FDCAN_T0_RTR_Pos | 168 << FDCAN_T0_11_bit_ID_Pos; // 11 бит ID, отправить данные, ID = 168

	uint32_t message_t1 = 0;
	message_t1 = 7 << FDCAN_T1_MM_Pos | 0 << FDCAN_T1_EFC_Pos | 0 << FDCAN_T1_FDF_Pos | 0 << FDCAN_T1_BRS_Pos | 8 << FDCAN_T1_DLC_Pos;// 7 - идентификатор сообщения, которое мы не храним, отправляем кадр в классическом CAN формате, 8 байта данных

	uint32_t message_t2 = 45123412;

//	// Данные подготовил, теперь надо записать их в RX FIFO
//	*(uint32_t *)(FDCAN_TX_BUFFER_Pos) = message_t0;
//	*(uint32_t *)(FDCAN_TX_BUFFER_Pos + 4) = message_t1;
//	*(uint32_t *)(FDCAN_TX_BUFFER_Pos + 8) = message_t2;
//	*(uint32_t *)(FDCAN_TX_BUFFER_Pos + 12) = 0;
//	*(uint32_t *)(FDCAN_TX_BUFFER_Pos + 16) = 0;
//	FDCAN1->TXBAR |= 1; // Добавляем новый запрос на отправку данных для TX буфера номер 1.

//	basic_uart2_setup(19200);


	while (1)
	{

		GPIOB->ODR ^= 0x60;
//		uart2_send_byte(54);

		delay_in_milliseconds(500);

		//	// Данные подготовил, теперь надо записать их в RX FIFO
		*(uint32_t *)(FDCAN_TX_BUFFER_Pos + FDCAN_TX_BUFFER_RECORD_SIZE * data_position) = message_t0;
		*(uint32_t *)(FDCAN_TX_BUFFER_Pos + FDCAN_TX_BUFFER_RECORD_SIZE * data_position + 4) = message_t1;
		*(uint32_t *)(FDCAN_TX_BUFFER_Pos + FDCAN_TX_BUFFER_RECORD_SIZE * data_position + 8) = message_t2;
		*(uint32_t *)(FDCAN_TX_BUFFER_Pos + FDCAN_TX_BUFFER_RECORD_SIZE * data_position + 12) = 0;
		*(uint32_t *)(FDCAN_TX_BUFFER_Pos + FDCAN_TX_BUFFER_RECORD_SIZE * data_position + 16) = 0;
		FDCAN1->TXBAR |= 1; // Добавляем новый запрос на отправку данных для TX буфера номер 1.


		data_position += 1;
		if(data_position == 3)
		{
			data_position = 0;
		}

		data_from_can = *(uint32_t *)(FDCAN_RX_FIFO_0_Pos + 8);


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


