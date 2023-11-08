// Copyright (c) 2023 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
* @file     button.h
* @brief    Button manipulations
* @author   Ziga Miklosic
* @email    ziga.miklosic@gmail.com
* @date     08.11.2023
* @version  V1.2.0
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

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 *     Module version
 */
#define BUTTON_VER_MAJOR        ( 1 )
#define BUTTON_VER_MINOR        ( 2 )
#define BUTTON_VER_DEVELOP      ( 0 )

/**
 *     Button status
 */
typedef enum
{
    eBUTTON_OK          = 0x00U,    /**<Normal operation */
    eBUTTON_ERROR_INIT  = 0x01U,    /**<Initialization error */
    eBUTTON_ERROR       = 0x02U,    /**<General error */
} button_status_t;

/**
 *  Button state
 */
typedef enum
{
    eBUTTON_OFF = 0,    /**<Button idle - not pressed */
    eBUTTON_ON,         /**<Button pressed */
    eBUTTON_UNKNOWN,    /**<During startup */
} button_state_t;

/**
 *     Callback functions
 */
typedef void(*pf_button_callback)(void);

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////
button_status_t button_init                 (void);
button_status_t button_deinit               (void);
button_status_t button_is_init              (bool * const p_is_init);
button_status_t button_hndl                 (void);
button_status_t button_get_state            (const button_num_t num, button_state_t * const p_state);
button_status_t button_get_time             (const button_num_t num, float32_t * const p_active_time, float32_t * const p_idle_time);
button_status_t button_set_enable           (const button_num_t num, const bool enable);
button_status_t button_get_enable           (const button_num_t num, bool * const p_enable);
button_status_t button_register_callback    (const button_num_t num, pf_button_callback pf_pressed, pf_button_callback pf_released);
button_status_t button_unregister_callback  (const button_num_t num);

#if ( 1 == BUTTON_CFG_FILTER_EN )
    button_status_t button_reset_filter     (const button_num_t num);
    button_status_t button_change_filter_fc (const button_num_t num, const float32_t fc);
#endif

#endif // __BUTTON_H_

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
