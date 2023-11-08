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
 * @addtogroup BUTTON
 * @{ <!-- BEGIN GROUP -->
 */
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "button.h"
#include "drivers/peripheral/gpio/gpio/src/gpio.h"

#if ( 1 == BUTTON_CFG_FILTER_EN )
    #include "middleware/filter/src/filter.h"
#endif

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

#if ( 1 == BUTTON_CFG_FILTER_EN )

    /**
     *  Compatibility check with Filter module
     *
     *  Support version V2.x.x
     */
    _Static_assert( 2 == FILTER_VER_MAJOR );

#endif

/**
 *     Button handler period
 *
 *     Unit: sec
 */
#define BUTTON_HNDL_PERIOD_S                ( BUTTON_CFG_HNDL_PERIOD_S )

/**
 *     Button handler frequency
 *
 *     Unit: Hz
 */
#define BUTTON_HNDL_FREQ_HZ                 ( 1.0f / BUTTON_HNDL_PERIOD_S )

/**
 *     Limit press time
 */
#define BUTTON_LIM_TIME( time )             (( time > 1E6f ) ? ( 1E6f ) : ( time ))

/**
 *     Button live data
 */
typedef struct
{
    #if ( 1 == BUTTON_CFG_FILTER_EN )
        p_filter_bool_t filt;       /**<Boolean filter */
    #endif
    pf_button_callback pressed;     /**<Button pressed callback */
    pf_button_callback released;    /**<Button released callback */

    struct
    {
        float32_t active;   /**<Button press-active time */
        float32_t idle;     /**<Button idle time */
    } time;

    struct
    {
        button_state_t cur;     /**<Current button state */
        button_state_t prev;    /**<Previous button state */
    } state;
    
    bool enable;    /**<Button enable switch */

} button_data_t;

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/**
 *     Initialisation guard
 */
static bool gb_is_init = false;

/**
 *     Pointer to configuration table
 */
static const button_cfg_t * gp_cfg_table = NULL;

/**
 *     Button data
 */
static button_data_t g_button[eBUTTON_NUM_OF] = { 0 };

////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
////////////////////////////////////////////////////////////////////////////////
static button_status_t  button_check_drv_init   (void);
static button_status_t  button_internal_init    (void);
static button_state_t   button_get_low          (const button_num_t num);
static button_state_t   button_filter_update    (const button_num_t num, const button_state_t state);
static void             button_raise_callback   (const button_num_t num, const button_state_t state_cur, button_state_t state_prev);
static void             button_manage_timings   (const button_num_t num, const button_state_t state);

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*       Check that low level drivers are initialised
*
* @return       status - Status of low level initialisation
*/
////////////////////////////////////////////////////////////////////////////////
static button_status_t button_check_drv_init(void)
{
    button_status_t status             = eBUTTON_OK;
    bool             gpio_drv_init     = false;

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
*       Initialize internal button data
*
* @return   status - Status of initialization
*/
////////////////////////////////////////////////////////////////////////////////
static button_status_t button_internal_init(void)
{
    button_status_t status = eBUTTON_OK;

    // Set up button
    for ( button_num_t num = 0; num < eBUTTON_NUM_OF; num++ )
    {
        // Init runtime vars
        g_button[num].pressed       = NULL;
        g_button[num].released      = NULL;
        g_button[num].time.active   = 0.0f;
        g_button[num].time.idle     = 0.0f;
        g_button[num].state.cur     = eBUTTON_UNKNOWN;
        g_button[num].state.prev    = eBUTTON_UNKNOWN;

        // Enable all buttons by default
        g_button[num].enable = true;

        #if ( 1 == BUTTON_CFG_FILTER_EN )

            g_button[num].filt = NULL;

            // Filter enable?
            if ( true == gp_cfg_table[num].lpf_en )
            {
                // Init filter
                // NOTE: Comparator level set to 0.05 (5%/95%)---> 3*Tao is 95% for RC 1st order filter
                if ( eFILTER_OK != filter_bool_init( &( g_button[num].filt ), gp_cfg_table[num].lpf_fc, BUTTON_HNDL_FREQ_HZ, 0.05f ))
                {
                    BUTTON_PRINT( "BUTTON: LPF initialisation error at button number %d!", num );
                    BUTTON_ASSERT( 0 );
                    status = eBUTTON_ERROR_INIT;
                    break;
                }
            }

        #endif
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Get button low level
*
* @param[in]    num     - Button enumeration number
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static button_state_t button_get_low(const button_num_t num)
{
    button_state_t  state         = eBUTTON_UNKNOWN;
    gpio_state_t    gpio_state    = eGPIO_UKNOWN;

    // Get gpio state
    (void) gpio_get( gp_cfg_table[num].gpio_pin, &gpio_state );

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
*       Update boolean filter
*
* @param[in]    num     - Button enumeration number
* @param[in]    state   - Current state of button
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static button_state_t button_filter_update(const button_num_t num, const button_state_t state)
{
    button_state_t state_filt = eBUTTON_UNKNOWN;

    #if ( 1 == BUTTON_CFG_FILTER_EN )
        bool in  = false;
        bool out = false;

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
            (void) filter_bool_hndl( g_button[num].filt, in, &out );

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
    #endif
        {
            state_filt = state;
        }

    return state_filt;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Manage button callbacks
*
* @note     If button is disabled, callbacks will not be triggered!
*
* @param[in]    num         - Button enumeration number
* @param[in]    state_cur   - Current state of button
* @param[in]    state_prev  - Previous state of button
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static void button_raise_callback(const button_num_t num, const button_state_t state_cur, button_state_t state_prev)
{
    // Is button enabled
    if ( true == g_button[num].enable )
    {
        if     (    ( NULL != g_button[num].pressed )
            &&     ( eBUTTON_ON  == state_cur )
            &&     ( eBUTTON_OFF == state_prev ))
        {
            g_button[num].pressed();
        }

        if     (    ( NULL != g_button[num].released )
            &&     ( eBUTTON_OFF == state_cur )
            &&     ( eBUTTON_ON  == state_prev ))
        {
            g_button[num].released();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Manage button timings
*
* @note     If button is disabled, timings will resets!
*
* @param[in]    num        - Button enumeration number
* @param[in]    state    - Current state of button
* @return       status     - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static void button_manage_timings(const button_num_t num, const button_state_t state)
{
    // Is button enabled
    if ( true == g_button[num].enable )
    { 
        if ( eBUTTON_ON == state )
        {
            g_button[num].time.idle     = 0.0f;
            g_button[num].time.active   += BUTTON_HNDL_PERIOD_S;
            g_button[num].time.active   = BUTTON_LIM_TIME( g_button[num].time.active );
        }
        else if ( eBUTTON_OFF == state )
        {
            g_button[num].time.active   = 0.0f;
            g_button[num].time.idle     += BUTTON_HNDL_PERIOD_S;
            g_button[num].time.idle     = BUTTON_LIM_TIME( g_button[num].time.idle );
        }
        else
        {
            g_button[num].time.active   = 0.0f;
            g_button[num].time.idle     = 0.0f;
        }
    }
    else
    {
        g_button[num].time.active   = 0.0f;
        g_button[num].time.idle     = 0.0f;
    }
}

////////////////////////////////////////////////////////////////////////////////
/*!
 * @} <!-- END GROUP -->
 */
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup BUTTON_API
* @{ <!-- BEGIN GROUP -->
*
*     Following functions are part of API calls.
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*       Button initialization
*
* @return       status - Status of initialization
*/
////////////////////////////////////////////////////////////////////////////////
button_status_t button_init(void)
{
    button_status_t status = eBUTTON_OK;

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
                BUTTON_PRINT( "BUTTON: Low level drivers not initialised error!" );
                BUTTON_ASSERT( 0 );
                status = eBUTTON_ERROR_INIT;
            }
        }
        else
        {
            BUTTON_PRINT( "BUTTON: Low level drivers not initialised error!" );
            BUTTON_ASSERT( 0 );
            status = eBUTTON_ERROR_INIT;
        }
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Button de-nitialization
*
* @return       status - Status of de-init
*/
////////////////////////////////////////////////////////////////////////////////
button_status_t button_deinit(void)
{
    button_status_t status = eBUTTON_OK;

    if ( false == gb_is_init )
    {
        gb_is_init = false;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Get button initialisation state
*
* @param[out]   p_is_init   - Initialization state
* @return       status      - Status of initialization
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

////////////////////////////////////////////////////////////////////////////////
/**
*       Main button handler
*
*@note     This function shall be called with constant period of value
*          set in "button_cfg.h" with macro "BUTTON_CFG_HNDL_PERIOD_S".
*
* @return       status - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
button_status_t button_hndl(void)
{
    button_status_t status = eBUTTON_OK;

    BUTTON_ASSERT( true == gb_is_init );

    if ( true == gb_is_init )
    {
        // For each button
        for ( button_num_t num = 0; num < eBUTTON_NUM_OF; num++ )
        {
            // Get button state
            const button_state_t state = button_get_low( num );

            // Apply filter
            g_button[num].state.cur = button_filter_update( num, state );

            // Manage callbacks
            button_raise_callback( num, g_button[num].state.cur, g_button[num].state.prev );

            // Manage timings
            button_manage_timings( num, g_button[num].state.cur );

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
*       Get current button state
*
* @note In case button is disabled, "eBUTTON_UNKNOWN" is returned!
*
* @param[in]    num     - Button enumeration number
* @param[out]   p_state - Pointer to current button state
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
button_status_t button_get_state(const button_num_t num, button_state_t * const p_state)
{
    button_status_t status = eBUTTON_OK;

    BUTTON_ASSERT( true == gb_is_init );
    BUTTON_ASSERT( num < eBUTTON_NUM_OF );
    BUTTON_ASSERT( NULL != p_state );

    if ( true == gb_is_init )
    {
        if ( true == g_button[num].enable )
        {
            if  (   ( num < eBUTTON_NUM_OF )
                &&  ( NULL != p_state ))
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
            *p_state = eBUTTON_UNKNOWN;     
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
*       Get button timings
*
* @param[in]    num             - Button enumeration number
* @param[out]   p_active_time   - Pointer to button active time
* @param[out]   p_idle_time     - Pointer to button idle time
* @return       status          - Status of operation
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
*       Enable/Disable button reading
*
* @param[in]    num     - Button enumeration number
* @param[in]    enable  - Enable/Disable button reading
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
button_status_t button_set_enable(const button_num_t num, const bool enable)
{
    button_status_t status = eBUTTON_OK;

    BUTTON_ASSERT( true == gb_is_init );
    BUTTON_ASSERT( num < eBUTTON_NUM_OF );

    if ( true == gb_is_init )
    {
        if ( num < eBUTTON_NUM_OF )
        {
            g_button[num].enable = enable;
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
*       Get enable button state
*
* @param[in]    num         - Button enumeration number
* @param[out]   p_enable    - State of enable button switch
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
button_status_t button_get_enable(const button_num_t num, bool * const p_enable)
{
    button_status_t status = eBUTTON_OK;

    BUTTON_ASSERT( true == gb_is_init );
    BUTTON_ASSERT( num < eBUTTON_NUM_OF );
    BUTTON_ASSERT( NULL != p_enable );

    if ( true == gb_is_init )
    {
        if  (   ( num < eBUTTON_NUM_OF )
            &&  ( NULL != p_enable ))
        {
            *p_enable = g_button[num].enable;
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
*       Register button press and release callbacks
*
* @brief    It is allowed to pass NULL pointer for if only one callback
*           functions is needed!
*
* @param[in]    num         - Button enumeration number
* @param[in]    pf_pressed  - Pointer to pressed callback function
* @param[in]    pf_released - Pointer to release callback function
* @return       status      - Status of operation
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
            g_button[num].pressed   = pf_pressed;
            g_button[num].released  = pf_released;

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
*       Un-register button press and release callbacks
*
* @param[in]    num     - Button enumeration number
* @return       status  - Status of operation
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
            g_button[num].pressed   = NULL;
            g_button[num].released  = NULL;
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

#if ( 1 == BUTTON_CFG_FILTER_EN )

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *       Reset button de-bouncing filter
    *
    * @note     This function is recommended to be called after enabling
    *           button reading back from disabled state!
    *
    * @param[in]    num     - Button enumeration number
    * @return       status  - Status of operation
    */
    ////////////////////////////////////////////////////////////////////////////////
    button_status_t button_reset_filter(const button_num_t num)
    {
        button_status_t status = eBUTTON_OK;

        BUTTON_ASSERT( true == gb_is_init );
        BUTTON_ASSERT( num < eBUTTON_NUM_OF );

        if ( true == gb_is_init )
        {
            if ( num < eBUTTON_NUM_OF )
            {
                // Reset filter
                if ( eFILTER_OK != filter_bool_reset( g_button[num].filt ))
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
    *       Change button filter cutoff frequency
    *
    * @brief    LPF is RC 1st order IIR filter. Output is being compared between
    *           5% and 95% (3 Tao).
    *
    *           E.g. LPF with fc=1Hz will result in button detection time delay
    *           of:
    *
    *                 fc = 1 / ( 2*pi*Tao ) ---> Tao = 1 / ( 2*pi*fc )
    *
    *                 dt = 3 Tao = 3 * 1 / ( 2*pi*fc )
    *
    *                 fc = 1.0 Hz ---> dt = 0.477 sec
    *
    *
    * @param[in]    num     - Button enumeration number
    * @param[in]    fc      - Filter cutoff frequency
    * @return       status  - Status of operation
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
            if  (   ( num < eBUTTON_NUM_OF )
                &&  ( true == gp_cfg_table[num].lpf_en ))
            {
                // Change cutoff frequency
                if ( eFILTER_OK != filter_bool_fc_set( g_button[num].filt, fc ))
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

#endif

////////////////////////////////////////////////////////////////////////////////
/*!
 * @} <!-- END GROUP -->
 */
////////////////////////////////////////////////////////////////////////////////
