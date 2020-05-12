#ifndef IMPLEMENTATION_H_
#define IMPLEMENTATION_H_

#include "implementaion_mistakes_codes.h"


/*

 */

// Утсанавливает частоту процессора
#ifndef SYSCLK_FREQUENCY
	#define SYSCLK_FREQUENCY 				48000000 // Hz = 24 Mhz
#endif /* SYSCLK_FREQUENCY */



//****************************************//
//****** Non user-adjustable defines ******//
//****************************************//
/*
	@brief All defines that should not be changed contains here. Also all checks for #define mistakes happen here redefines happen here so they will be at top of any listing and won't distract programmers.
 */

/* Check for correct system clock define  */
#if (SYSCLK_FREQUENCY > 64000000)
	#error Clock speed define is higher than maximum value of 64 Mhz

#elif (SYSCLK_FREQUENCY < 16000000)
	#error Clock speed define is less then minimum value of 16 Mhz

#endif


// @brief Mistakes log entry form
typedef struct{
	uint32_t mistake_code;
	uint32_t mistake_time_in_milliseconds;
	uint32_t mistake_time_in_seconds;
} mistake;

/*** Global variables declaration ***/
/*
	@brief Declares global variables in all files that include this one.

	 If there is VAR_DECLS define in file - declares and initialize variables (should be only one such file).
	 If there is no VAR_DECLS define in file - declares all variables as extern and doesn't initialize them.
 */
#ifndef VAR_DEFS
#define VAR_DEFS 1		// Make sure this file is included only once for each .c file

#ifndef VAR_DECLS		// There should be one file with defined VAR_DECLS. So there will be only one file with variables initialization.
# define _DECL extern	// If the .c file doesn't contains VAR_DECLS definition, all global variables will be declared with extern prefix and without initialization
# define _INIT(x)
#else
# define _DECL			// If there is VAR_DECLS definition in .c file, all global variables will be declared without extern prefix
# define _INIT(x)  = x	// and will be initialized with predefined values x
#endif /* VAR_DECLS */

/* Global variables should be declared as follows: "_DECL [standard variable declaration] _INIT(x);", where x is initialization value.
 * If there is no need for variables initialization, declaration can look as: "_DECL [standard variable declaration];" */

///*** Mistakes log variables ***//
//	@brief This log is used to accumulate mistakes, that occurred during runtime, so it will be possible to debug system with debugger.
_DECL mistake mistakes_log[MISTAKES_LOG_SIZE];
_DECL uint32_t mistakes_log_pointer _INIT(0);
_DECL uint32_t critical_mistake_detected _INIT(0);		/* will be 1 - if there were a critical mistake. System should be stopped. All performance critical objects should be returned to safe state and be deactivated */
_DECL uint32_t time_from_log_enable_in_seconds _INIT(0);

_DECL uint32_t delay_is_finished _INIT(0);

#endif /* VAR_DEFS */
/******************************************/

enum question_answers
{
	no = 0,
	yes = 1
};

//*** GPIO setup defines ***//
#define GPIO_MODER_MSK			(3U) 	//0x11
#define GPIO_ANALOG_IN 			(3U)	//0x11
#define GPIO_ALTERNATE			(2U)	//0x10
#define GPIO_DIGITAL_OUT		(1U)	//0x01
#define GPIO_DIGITAL_IN			(0U)	//0x00

#define GPIO_OSPEED_VERY_LOW	(0U)	//0x00
#define GPIO_OSPEED_LOW			(1U)	//0x01
#define GPIO_OSPEED_HIGH		(2U)	//0x10
#define GPIO_OSPEED_VERY_HIGH	(3U)	//0x11

#define ALTERNATE_FUNCTION_1	(1U)	//0x01
#define ALTERNATE_FUNCTION_2	(2U)	//0x10

//*** System clock prescalers ***//
#define	PLLN_VALUE  			SYSCLK_FREQUENCY/2000000
#define PLLR_VALUE				4
#define PLL_OFFSET				1					// Offset  for PLLM and PLLR
#define PLLM_VALUE_WITH_HSI		2

//*** HSE state defines ***//
#define HSE_IS_OK 		1
#define HSE_IS_NOT_OK 	0

/*** Completely random value to determine the waiting-state length ***/
#define DUMMY_DELAY_VALUE 10000


#endif /* IMPLEMENTATION_H_ */
