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
#include <stdint.h>
#include <stdbool.h>

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
#define BUTTON_HNDL_PERIOD_S			( BUTTON_CFG_HNDL_PERIOD_S )

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
 * 	Button live data
 */
typedef struct
{
	p_filter_bool_t		filt;		/**<Boolean filter */
	pf_button_callback 	pressed;	/**<Button pressed callback */
	pf_button_callback 	released;	/**<Button released callback */

	struct
	{
		float32_t active;			/**<Button press-active time */
		float32_t idle;				/**<Button idle time */
	} time;

	struct
	{
		button_state_t	cur;		/**<Current button state */
		button_state_t	prev;		/**<Previous button state */
	} state;

} button_data_t;

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/**
 * 	Initialisation guard
 */
static bool gb_is_init = false;

/**
 * 	Pointer to configuration table
 */
static const button_cfg_t * gp_cfg_table = NULL;

/**
 * 	Button data
 */
static button_data_t g_button[eBUTTON_NUM_OF] = { 0 };

////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
////////////////////////////////////////////////////////////////////////////////
static button_status_t 	button_check_drv_init	(void);
static button_status_t 	button_internal_init	(void);
static button_state_t	button_get_low			(const button_num_t num);
static button_state_t	button_filter_update	(const button_num_t num, const button_state_t state);
static void				button_raise_callback	(const button_num_t num, const button_state_t state_cur, button_state_t state_prev);
static void				button_manage_timings	(const button_num_t num, const button_state_t state);

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
*   	Check that low level drivers are initialised
*
* @return   	status - Status of low level initialisation
*/
////////////////////////////////////////////////////////////////////////////////
static button_status_t button_check_drv_init(void)
{
	button_status_t status 			= eBUTTON_OK;
	bool 			gpio_drv_init 	= false;

	// Get init flag
	gpio_is_init( &gpio_drv_init );

	if ( false == gpio_drv_init )
	{
		status = eBUTTON_ERROR_INIT;
	}

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*   	Initialise internal button data
*
* @return   status - Status of initialisation
*/
////////////////////////////////////////////////////////////////////////////////
static button_status_t button_internal_init(void)
{
	button_status_t status 	= eBUTTON_OK;
	button_num_t	num		= 0;

	// Set up button
	for ( num = 0; num < eBUTTON_NUM_OF; num++ )
	{
		// Init runtime vars
		g_button[num].filt 			= NULL;
		g_button[num].pressed		= NULL;
		g_button[num].released		= NULL;
		g_button[num].time.active 	= 0.0f;
		g_button[num].time.idle	 	= 0.0f;
		g_button[num].state.cur		= eBUTTON_UNKNOWN;
		g_button[num].state.prev	= eBUTTON_UNKNOWN;

		// Filter enable?
		if ( true == gp_cfg_table[num].lpf_en )
		{
			// Init filter
			if ( eFILTER_OK != filter_bool_init( &( g_button[num].filt ), gp_cfg_table[num].lpf_fc, BUTTON_HNDL_FREQ_HZ, 0.05f ))
			{
				BUTTON_DBG_PRINT( "BUTTON: LPF initialisation error at button number %d!", num );
				BUTTON_ASSERT( 0 );
				status = eBUTTON_ERROR_INIT;
				break;
			}
		}
	}

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*   	Get button low level
*
* @param[in]	num		- Button enumeration number
* @return   	status 	- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static button_state_t button_get_low(const button_num_t num)
{
	button_state_t 	state 		= eBUTTON_UNKNOWN;
	gpio_state_t	gpio_state	= eGPIO_UKNOWN;

	// Get gpio state
	gpio_state = gpio_get( gp_cfg_table[num].gpio_pin );

	// Active high polarity
	if ( eBUTTON_POL_ACTIVE_HIGH == gp_cfg_table[num].polarity )
	{
		if ( eGPIO_HIGH == gpio_state )
		{
			state = eBUTTON_ON;
		}
		else if ( eGPIO_LOW == gpio_state )
		{
			state = eBUTTON_OFF;
		}
		else
		{
			state = eBUTTON_UNKNOWN;
		}
	}

	// Active low polarity
	else
	{
		if ( eGPIO_HIGH == gpio_state )
		{
			state = eBUTTON_OFF;
		}
		else if ( eGPIO_LOW == gpio_state )
		{
			state = eBUTTON_ON;
		}
		else
		{
			state = eBUTTON_UNKNOWN;
		}
	}

	return state;
}

////////////////////////////////////////////////////////////////////////////////
/**
*   	Update boolean filter
*
* @param[in]	num		- Button enumeration number
* @param[in]	state	- Current state of button
* @return   	status 	- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static button_state_t button_filter_update(const button_num_t num, const button_state_t state)
{
	button_state_t 	state_filt 	= eBUTTON_UNKNOWN;
	bool			in 			= false;
	bool			out 		= false;

	// Filter enabled
	if ( true == gp_cfg_table[num].lpf_en )
	{
		// Convert state
		if ( eBUTTON_ON == state )
		{
			in = true;
		}
		else
		{
			in = false;
		}

		// Update filter
		filter_bool_update( g_button[num].filt, in, &out );

		// Convert state
		if ( true == out )
		{
			state_filt = eBUTTON_ON;
		}
		else
		{
			state_filt = eBUTTON_OFF;
		}
	}

	// Filter disabled
	else
	{
		state_filt = state;
	}

	return state_filt;
}

////////////////////////////////////////////////////////////////////////////////
/**
*   	Manage button callbacks
*
* @param[in]	num			- Button enumeration number
* @param[in]	state_cur	- Current state of button
* @param[in]	state_prev	- Previous state of button
* @return   	status 		- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static void	button_raise_callback(const button_num_t num, const button_state_t state_cur, button_state_t state_prev)
{
	if 	(	( NULL != g_button[num].pressed )
		&& 	( eBUTTON_ON  == state_cur )
		&& 	( eBUTTON_OFF == state_prev ))
	{
		g_button[num].pressed();
	}

	if 	(	( NULL != g_button[num].released )
		&& 	( eBUTTON_OFF == state_cur )
		&& 	( eBUTTON_ON  == state_prev ))
	{
		g_button[num].released();
	}
}

////////////////////////////////////////////////////////////////////////////////
/**
*   	Manage button timings
*
* @param[in]	num		- Button enumeration number
* @param[in]	state	- Current state of button
* @return   	status 	- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static void	button_manage_timings(const button_num_t num, const button_state_t state)
{
	if ( eBUTTON_ON == state )
	{
		g_button[num].time.idle 	= 0.0f;
		g_button[num].time.active 	+= BUTTON_HNDL_PERIOD_S;
		g_button[num].time.active 	= BUTTON_LIM_TIME( g_button[num].time.active );
	}
	else if ( eBUTTON_OFF == state )
	{
		g_button[num].time.active 	= 0.0f;
		g_button[num].time.idle 	+= BUTTON_HNDL_PERIOD_S;
		g_button[num].time.idle 	= BUTTON_LIM_TIME( g_button[num].time.idle );
	}
	else
	{
		g_button[num].time.active 	= 0.0f;
		g_button[num].time.idle 	= 0.0f;
	}
}

////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup BUTTON_API
* @{ <!-- BEGIN GROUP -->
*
* 	Following functions are part of API calls.
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*   	Button initialisation
*
* @return   	status - Status of low level initialisation
*/
////////////////////////////////////////////////////////////////////////////////
button_status_t	button_init(void)
{
	button_status_t status 	= eBUTTON_OK;

	BUTTON_ASSERT( false == gb_is_init );

	if ( false == gb_is_init )
	{
		// Get configuration table
		gp_cfg_table = button_cfg_get_table();

		if ( NULL != gp_cfg_table )
		{
			// Check low level drivers
			if ( eBUTTON_OK == button_check_drv_init())
			{
				// Init internal button data
				status = button_internal_init();

				// Init success
				if ( eBUTTON_OK == status )
				{
					gb_is_init = true;
				}
			}

			// Low level drivers not initialised
			else
			{
				BUTTON_DBG_PRINT( "BUTTON: Low level drivers not initialised error!" );
				BUTTON_ASSERT( 0 );
				status = eBUTTON_ERROR_INIT;
			}
		}
		else
		{
			BUTTON_DBG_PRINT( "BUTTON: Low level drivers not initialised error!" );
			BUTTON_ASSERT( 0 );
			status = eBUTTON_ERROR_INIT;
		}
	}
	else
	{
		BUTTON_DBG_PRINT( "BUTTON: Already init ERROR!" );
		BUTTON_ASSERT( 0 );
		status = eBUTTON_ERROR_INIT;
	}

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*   Get button initialisation flag
*
* @param[out]	p_is_init 	- Initialisation flag
* @return   	status		- Status of initialisation
*/
////////////////////////////////////////////////////////////////////////////////
button_status_t button_is_init(bool * const p_is_init)
{
	button_status_t status = eBUTTON_OK;

	if ( NULL != p_is_init )
	{
		*p_is_init = gb_is_init;
	}
	else
	{
		status = eBUTTON_ERROR;
	}

	return status;
}

button_status_t button_hndl(void)
{
	button_status_t status 		= eBUTTON_OK;
	button_num_t	num			= 0;
	button_state_t	state		= eBUTTON_UNKNOWN;

	BUTTON_ASSERT( true == gb_is_init );

	if ( true == gb_is_init )
	{
		// For each button
		for ( num = 0; num < eBUTTON_NUM_OF; num++ )
		{
			// Get button state
			state = button_get_low( num );

			// Apply filter
			g_button[num].state.cur = button_filter_update( num, state );

			// Manage timings
			button_manage_timings( num, g_button[num].state.cur );

			// Manage callbacks
			button_raise_callback( num, g_button[num].state.cur, g_button[num].state.prev );

			// Store current state
			g_button[num].state.prev = g_button[num].state.cur;
		}
	}
	else
	{
		status = eBUTTON_ERROR_INIT;
	}

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*   	Get current button state
*
* @param[in]	num		- Button enumeration number
* @param[out]	p_state	- Pointer to current button state
* @return   	status	- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
button_status_t button_get_state(const button_num_t num, bool * const p_state)
{
	button_status_t status = eBUTTON_OK;

	BUTTON_ASSERT( true == gb_is_init );
	BUTTON_ASSERT( num < eBUTTON_NUM_OF );
	BUTTON_ASSERT( NULL != p_state );

	if ( true == gb_is_init )
	{
		if 	(	( num < eBUTTON_NUM_OF )
			&& 	( NULL != p_state ))
		{
			*p_state = g_button[num].state.cur;
		}
		else
		{
			status = eBUTTON_ERROR;
		}
	}
	else
	{
		status = eBUTTON_ERROR_INIT;
	}

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*   	Change button filter cutoff frequency
*
*	@brief	LPF is RC 1st order IIR filter. Output is being compared between
* 			5% and 95% (3 Tao).
*
* 			E.g. LPF with fc=1Hz will result in button detection time delay
* 			of:
*
* 				fc = 1 / ( 2*pi*Tao ) ---> Tao = 1 / ( 2*pi*fc )
*
* 				dt = 3 Tao = 3 * 1 / ( 2*pi*fc )
*
* 				fc = 1.0 Hz ---> dt = 0.477 sec
*
*
* @param[in]	num		- Button enumeration number
* @param[in]	fc		- Filter cutoff frequency
* @return   	status	- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
button_status_t button_change_filter_fc(const button_num_t num, const float32_t fc)
{
	button_status_t status = eBUTTON_OK;

	BUTTON_ASSERT( true == gb_is_init );
	BUTTON_ASSERT( num < eBUTTON_NUM_OF );
	BUTTON_ASSERT( true == gp_cfg_table[num].lpf_en );

	if ( true == gb_is_init )
	{
		if 	(	( num < eBUTTON_NUM_OF )
			&& 	( true == gp_cfg_table[num].lpf_en ))
		{
			// Change cutoff frequency
			if ( eFILTER_OK != filter_bool_change_cutoff( g_button[num].filt, fc, BUTTON_HNDL_FREQ_HZ ))
			{
				status = eBUTTON_ERROR;
			}
		}
		else
		{
			status = eBUTTON_ERROR;
		}
	}
	else
	{
		status = eBUTTON_ERROR_INIT;
	}

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*   	Get button timings
*
* @param[in]	num				- Button enumeration number
* @param[out]	p_active_time	- Pointer to button active time
* @param[out]	p_idle_time		- Pointer to button idle time
* @return   	status			- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
button_status_t button_get_time(const button_num_t num, float32_t * const p_active_time, float32_t * const p_idle_time)
{
	button_status_t status = eBUTTON_OK;

	BUTTON_ASSERT( true == gb_is_init );
	BUTTON_ASSERT( num < eBUTTON_NUM_OF );

	if ( true == gb_is_init )
	{
		if ( num < eBUTTON_NUM_OF )
		{
			if ( NULL != p_active_time )
			{
				*p_active_time = g_button[num].time.active;
			}

			if ( NULL != p_idle_time )
			{
				*p_idle_time = g_button[num].time.idle;
			}
		}
		else
		{
			status = eBUTTON_ERROR;
		}
	}
	else
	{
		status = eBUTTON_ERROR_INIT;
	}

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*   	Register button press and release callbacks
*
* @param[in]	num				- Button enumeration number
* @param[in]	pf_pressed		- Pointer to pressed callback function
* @param[in]	pf_released		- Pointer to release callback function
* @return   	status			- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
button_status_t button_register_callback(const button_num_t num, pf_button_callback pf_pressed, pf_button_callback pf_released)
{
	button_status_t status = eBUTTON_OK;

	BUTTON_ASSERT( true == gb_is_init );
	BUTTON_ASSERT( num < eBUTTON_NUM_OF );

	if ( true == gb_is_init )
	{
		if ( num < eBUTTON_NUM_OF )
		{
			// Checking for NULL at call
			g_button[num].pressed 	= pf_pressed;
			g_button[num].released 	= pf_released;

		}
		else
		{
			status = eBUTTON_ERROR;
		}
	}
	else
	{
		status = eBUTTON_ERROR_INIT;
	}

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*   	Un-register button press and release callbacks
*
* @param[in]	num				- Button enumeration number
* @return   	status			- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
button_status_t button_unregister_callback(const button_num_t num)
{
	button_status_t status = eBUTTON_OK;

	BUTTON_ASSERT( true == gb_is_init );
	BUTTON_ASSERT( num < eBUTTON_NUM_OF );

	if ( true == gb_is_init )
	{
		if ( num < eBUTTON_NUM_OF )
		{
			g_button[num].pressed 	= NULL;
			g_button[num].released 	= NULL;
		}
		else
		{
			status = eBUTTON_ERROR;
		}
	}
	else
	{
		status = eBUTTON_ERROR_INIT;
	}

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
 * @} <!-- END GROUP -->
 */
////////////////////////////////////////////////////////////////////////////////
