// Copyright (c) 2021 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
* @file     button.h
* @brief    Button manipulations
* @author   Ziga Miklosic
* @date     27.09.2021
*/
////////////////////////////////////////////////////////////////////////////////
/**
* @addtogroup LED_API
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
#include "project_config.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

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
	eBUTTON_ON		= 1,		/**<Button pressed */
	eBUTTON_UNKNOWN = 2,		/**<During startup */
} button_state_t;

/**
 * 	End-switches
 */
typedef enum
{
	eBUTTON_RE = 0,

	eBUTTON_END_SW_NUM_OF,
} button_end_sw_opt_t;

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////
button_status_t	button_init					(void);
void            button_hndl                 (void);
button_state_t  button_get_state            (void);
void            button_pressed_callback     (void);
void            button_release_callback     (void);
float32_t		button_get_press_time		(void);
bool			button_get_end_sw			(const button_end_sw_opt_t sw);
void			button_change_filter_fc		(const float32_t fc);

#endif // __BUTTON_H_
////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
