// Copyright (c) 2022 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
* @file     button.h
* @brief    Button manipulations
* @author   Ziga Miklosic
* @date     04.11.2022
* @version	V1.1.0
*/
////////////////////////////////////////////////////////////////////////////////
/**
* @addtogroup BUTTON_API
* @{ <!-- BEGIN GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

#ifndef __BUTTON_H_
#define __BUTTON_H_

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>

#include "../../button_cfg.h"
#include "drivers/peripheral/gpio/gpio.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 * 	Module version
 */
#define BUTTON_VER_MAJOR		( 1 )
#define BUTTON_VER_MINOR		( 1 )
#define BUTTON_VER_DEVELOP		( 0 )

/**
 * 	Button status
 */
typedef enum
{
	eBUTTON_OK 			= 0,		/**<Normal operation */
	eBUTTON_ERROR_INIT 	= 0x01,		/**<Initialization error */
	eBUTTON_ERROR		= 0x02,		/**<General error */
} button_status_t;

/**
 *  Button state
 */
typedef enum
{
    eBUTTON_OFF 	= 0,		/**<Button idle - not pressed */
	eBUTTON_ON,					/**<Button pressed */
	eBUTTON_UNKNOWN,			/**<During startup */
} button_state_t;

/**
 * 	Button polarity
 */
typedef enum
{
	eBUTTON_POL_ACTIVE_HIGH = 0,	/**<Active high polarity */
	eBUTTON_POL_ACTIVE_LOW,			/**<Active low polatiry */
} button_polarity_t;

/**
 *  32-bit floating data type definition
 */
typedef float float32_t;

/**
 * 	Button configuration
 */
typedef struct
{
	gpio_pins_t 		gpio_pin;		/**<GPIO pin */
	button_polarity_t	polarity;		/**<Polarity */
	bool				lpf_en;			/**<Enable LPF */
	float32_t			lpf_fc;			/**<Low pass filter cutoff freq */
} button_cfg_t;

/**
 * 	Callback functions
 */
typedef void(*pf_button_callback)(void);


////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////
button_status_t	button_init					(void);
button_status_t button_is_init				(bool * const p_is_init);
button_status_t button_hndl                 (void);
button_status_t button_get_state            (const button_num_t num, bool * const p_state);
button_status_t button_get_time				(const button_num_t num, float32_t * const p_active_time, float32_t * const p_idle_time);
button_status_t button_register_callback	(const button_num_t num, pf_button_callback pf_pressed, pf_button_callback pf_released);
button_status_t button_unregister_callback	(const button_num_t num);

#if ( 1 == BUTTON_CFG_FILTER_EN )
	button_status_t button_change_filter_fc		(const button_num_t num, const float32_t fc);
#endif

#endif // __BUTTON_H_
////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
