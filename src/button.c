// Copyright (c) 2021 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
* @file     button.h
* @brief    Button manipulations
* @author   Ziga Miklosic
* @date     27.09.2021
* @version	V1.0.0
*/
////////////////////////////////////////////////////////////////////////////////
/**
 * @addtogroup BUTTON_API
 * @{ <!-- BEGIN GROUP -->
 */
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include "button.h"
#include "drivers/peripheral/gpio/gpio.h"
#include "middleware/filter/src/filter.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 * 	Button handler period
 *
 * 	Unit: sec
 */
#define BUTTON_HNDL_PERIOD_S			( 0.01f )

/**
 * 	Button handler frequency
 *
 * 	Unit: Hz
 */
#define BUTTON_HNDL_FREQ_HZ				( 1.0f / BUTTON_HNDL_PERIOD_S )

/**
 * 	Limit press time
 */
#define BUTTON_LIM_TIME( time )			(( time > 1E6f ) ? ( 1E6f ) : ( time ))

/**
 * 	Default LPF fc
 *
 * 	Unit: Hz
 */
#define BUTTON_LPF_FC_HZ				( 1.0f )

/**
 * 	End-switch data
 */
typedef struct
{
	p_filter_bool_t	filt;			/**<Boolean filter */
	float32_t		press_time;		/**<Button press time */
	float32_t		idle_time;		/**<Button idle time */
	gpio_pins_t		pin;			/**<GPIO pin */
	bool			state;			/**<Current button state */
	bool			state_filt;		/**<Filtered button state */
} button_end_sw_t;

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/**
 * 	Button press time

static float32_t gf32_btn_press_time = 0.0f;

*
 * 	End-switch data

static button_end_sw_t g_end_sw[eBUTTON_END_SW_NUM_OF] =
{
	{  .filt = NULL, .press_time = 0, .idle_time = 0, .pin = eGPIO_RE_BTN, 	.state = false, .state_filt = false	},
};*/

////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
////////////////////////////////////////////////////////////////////////////////
/*static void button_user_hndl			(void);
static void button_end_sw_hndl			(void);
static void button_end_sw_manage_time	(const button_end_sw_opt_t sw);*/

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

#if 0
////////////////////////////////////////////////////////////////////////////////
/**
*       Button initilization
*
*@precondition  GPIO drivers must be initialized before calling this function
*
* @return   status - Status of initialization
*/
////////////////////////////////////////////////////////////////////////////////
button_status_t	button_init(void)
{
	button_status_t	status 	= eBUTTON_OK;
	uint32_t 		sw_num 	= 0UL;

	// Init LPF filer for buttons
	for ( sw_num = 0; sw_num < eBUTTON_END_SW_NUM_OF; sw_num++ )
	{
		if ( eFILTER_OK != filter_bool_init( &( g_end_sw[sw_num].filt ), BUTTON_LPF_FC_HZ, BUTTON_HNDL_FREQ_HZ, 0.2f ))
		{
			status = eBUTTON_ERROR_INIT;
			break;
		}
	}

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Button handler
*
*@precondition  GPIO drivers must be initialized before calling this function
*
* @return   void
*/
////////////////////////////////////////////////////////////////////////////////
void button_hndl(void)
{
	// User button - placed on Nucleo board
	button_user_hndl();

	// End-switch handler
	button_end_sw_hndl();
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Get button state
*
* @note 	Active low polarity
*
* @return   btn_state   - Either OFF or ON
*/
////////////////////////////////////////////////////////////////////////////////
button_state_t button_get_state(void)
{
    button_state_t  btn_state   = eBUTTON_OFF;
    gpio_state_t    gpio_state  = eGPIO_UKNOWN;

    // Get GPIO state
    gpio_state = gpio_get( eGPIO_USER_BTN );

    if ( eGPIO_HIGH == gpio_state )
    {
        btn_state = eBUTTON_OFF;
    }
    else
    {
        btn_state = eBUTTON_ON;
    }

    return btn_state;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Button pressed callback
*
*   This callback is being called on user button pressed action.
*
* @return   void
*/
////////////////////////////////////////////////////////////////////////////////
__attribute__((weak)) void button_pressed_callback(void)
{
    /**
     *@note Leave empty implementation here. User shall provide it's
     *      own definition of function if needed.
     *
     */
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Button release callback
*
*   This callback is being called on user button release action.
*
* @return   void
*/
////////////////////////////////////////////////////////////////////////////////
__attribute__((weak)) void button_release_callback(void)
{
    /**
     *@note Leave empty implementation here. User shall provide it's
     *      own definition of function if needed.
     *
     */
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Get button press state
*
* @return   Button total press time
*/
////////////////////////////////////////////////////////////////////////////////
float32_t button_get_press_time(void)
{
	return gf32_btn_press_time;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Get state of end-switch
*
* @param[in]	sw 		- End-switch number
* @return   	state	- Current state of end-switch
*/
////////////////////////////////////////////////////////////////////////////////
bool button_get_end_sw(const button_end_sw_opt_t sw)
{
	bool state = false;

	if ( sw < eBUTTON_END_SW_NUM_OF )
	{
		state = g_end_sw[sw].state_filt;
	}

	return state;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Change end-switch cutoff freq
*
* @param[in]	fc 		- End-switch filter cuttoff freq
* @return   	state	- Current state of end-switch
*/
////////////////////////////////////////////////////////////////////////////////
void button_change_filter_fc(const float32_t fc)
{
	uint32_t sw_num = 0UL;

	for ( sw_num = 0; sw_num < eBUTTON_END_SW_NUM_OF; sw_num++ )
	{
		filter_bool_change_cutoff( g_end_sw[sw_num].filt, fc, BUTTON_HNDL_FREQ_HZ );
	}
}

////////////////////////////////////////////////////////////////////////////////
/**
*       User button handler
*
* @return   void
*/
////////////////////////////////////////////////////////////////////////////////
static void button_user_hndl(void)
{
	static 	uint32_t		tick_prev		= 0UL;
			uint32_t		tick			= 0UL;
    static 	button_state_t 	btn_state_prev  = eBUTTON_OFF;
    		button_state_t 	btn_state       = eBUTTON_OFF;

    // Get current button state
    btn_state = button_get_state();

    // Button state changed
    if ( btn_state != btn_state_prev )
    {
        // Button pressed
        if ( eBUTTON_ON == btn_state )
        {
            button_pressed_callback();
        }

        // Button release
        else
        {
            button_release_callback();
        }
    }

    // Store state
    btn_state_prev = btn_state;

    // Manage press time
    tick = HAL_GetTick();

    if ( eBUTTON_ON == btn_state )
    {
    	gf32_btn_press_time += (float32_t)(( tick - tick_prev ) / 1000.0f );
    }
    else
    {
    	gf32_btn_press_time = 0.0f;
    }

    tick_prev = tick;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Get ens switch state
*
* @note 	Active low polarity
*
* @return   btn_state   - Either OFF or ON
*/
////////////////////////////////////////////////////////////////////////////////
static button_state_t button_get_state_end_sw(const button_end_sw_opt_t sw)
{
    button_state_t  btn_state   = eBUTTON_OFF;
    gpio_state_t    gpio_state  = eGPIO_UKNOWN;

    // Get GPIO state
    gpio_state = gpio_get( g_end_sw[sw].pin );

    if ( eGPIO_HIGH == gpio_state )
    {
        btn_state = eBUTTON_OFF;
    }
    else
    {
        btn_state = eBUTTON_ON;
    }

    return btn_state;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       End-Switch handler
*
* @brief	Performing filtering of switches
*
* @return   void
*/
////////////////////////////////////////////////////////////////////////////////
static void button_end_sw_hndl(void)
{
	uint32_t 	sw_num 		= 0UL;
	bool 		filt_val	= false;
	filter_status_t filt_status = eFILTER_OK;

	for ( sw_num = 0; sw_num < eBUTTON_END_SW_NUM_OF; sw_num++ )
	{
		// Get input
		g_end_sw[sw_num].state = button_get_state_end_sw( sw_num );

		// Apply filter
		filt_status = filter_bool_update( g_end_sw[sw_num].filt, (bool) g_end_sw[sw_num].state, &filt_val );

		if ( eFILTER_OK == filt_status )
		{
			g_end_sw[sw_num].state_filt = filt_val;
		}

		// Manage time
		button_end_sw_manage_time( sw_num );
	}
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Manage end-switch time
*
* @param[in]	sw 	- End switch
* @return   	void
*/
////////////////////////////////////////////////////////////////////////////////
static void button_end_sw_manage_time(const button_end_sw_opt_t sw)
{
	// Pressed
	if ( true == g_end_sw[sw].state_filt )
	{
		g_end_sw[sw].idle_time = 0.0f;
		g_end_sw[sw].press_time += BUTTON_HNDL_PERIOD_S;
		g_end_sw[sw].press_time = BUTTON_LIM_TIME( g_end_sw[sw].press_time );
	}

	// Idle
	else
	{
		g_end_sw[sw].press_time = 0.0f;
		g_end_sw[sw].idle_time += BUTTON_HNDL_PERIOD_S;
		g_end_sw[sw].idle_time = BUTTON_LIM_TIME( g_end_sw[sw].idle_time );
	}
}

#endif

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup BUTTON_API
* @{ <!-- BEGIN GROUP -->
*
* 	Following functions are part of API calls.
*/
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
/*!
 * @} <!-- END GROUP -->
 */
////////////////////////////////////////////////////////////////////////////////
