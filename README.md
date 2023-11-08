# **Button**
Button library takes care of button handling and debouncing if configured to do so. Library is fully configured via configuration table.

## **Dependencies**

### **1. GPIO module**
Button library is dependend from low level GPIO module thus following API functions must be provided:
```C
gpio_status_t gpio_is_init  (bool * const p_is_init)
gpio_status_t gpio_get      (const gpio_pin_t pin, gpio_state_t * const p_state)
```

GPIO module must take following project path:
```
"root/drivers/peripheral/gpio/gpio/src/gpio.h"
```

### **2. Filter module**
[Filter module](https://github.com/GeneralEmbeddedCLibraries/filter) is not mandatory to be used as it can be disabled via *BUTTON_CFG_FILTER_EN* inside ***button_cfg.h*** file.

If Filter module is used, it must take following project path:
```
"root/middleware/filter/src/filter.h"
```

## **General Embedded C Libraries Ecosystem**
In order to be part of *General Embedded C Libraries Ecosystem* this module must be placed in following path: 
```
root/drivers/hmi/button/button/"module_space"
```

 ## **API**
| API Functions | Description | Prototype |
| --- | ----------- | ----- |
| **button_init**                   | Initialization of button module           | button_status_t button_init(void) |
| **button_deinit**                 | De-initialization of button module        | button_status_t button_deinit(void) |
| **button_is_init**                | Get initialization flag                   | button_status_t button_is_init(bool * const p_is_init) |
| **button_hndl**                   | Main button handler                       | button_status_t button_hndl(void) |
| **button_get_state**              | Get button state                          | button_status_t button_get_state(const button_num_t num, bool * const p_state) |
| **button_change_filter_fc**       | Change button cutoff frequency of filter  | button_status_t button_change_filter_fc(const button_num_t num, const float32_t fc) |
| **button_get_time**               | Get button timings                        | button_status_t button_get_time(const button_num_t num, float32_t * const p_active_time, float32_t * const p_idle_time)|
| **button_register_callback**      | Register button callback                  | button_status_t button_register_callback(const button_num_t num, pf_button_callback pf_pressed, pf_button_callback pf_released) |
| **button_unregister_callback**    | Un-register button callback               | button_status_t button_unregister_callback(const button_num_t num) |


## **How to use**

**GENERAL NOTICE: Put all user code between sections: USER CODE BEGIN & USER CODE END!**

**1. Copy template files to root directory of module.**

**2. List all buttons inside **button_cfg.h** file:**
```C
/**
 *     List of Buttons
 *
 * @note     User shall provide Button name here as it would be using
 *             later inside code.
 *
 *             Shall always start with 0.
 *
 * @note     User shall change code only inside section of "USER_CODE_BEGIN"
 *             ans "USER_CODE_END".
 */
typedef enum
{
    // USER CODE START...

    // End-Switches for position detection
    eBUTTON_BUCKET_HOME_SW = 0,     /**<Bucket at home position switch */
    eBUTTON_DELIVERY_OPEN_SW,       /**<Delivery door open switch */
    eBUTTON_DELIVERY_CLOSE_SW,      /**<Delivery door close switch */

    // Motor driver digital status line debouncing
    eBUTTON_DELIVERY_FAULT,         /**<Delivery power stage (MPS) fault */
    eBUTTON_SLIDER_FAULT,           /**<Bridge power stage (MPS) fault */

    // USER CODE END...

    eBUTTON_NUM_OF
} button_num_t;
```

**3. Configure Button module inside **button_cfg.h** file:**

| Configuration | Description |
| --- | --- |
| **BUTTON_CFG_HNDL_PERIOD_S** 	| Main button handler period in seconds. |
| **BUTTON_CFG_FILTER_EN** 		| Enable/Disable usage of Filter module. It is being used for debouncing. |
| **BUTTON_CFG_DEBUG_EN** 		| Enable/Disable debug mode. |
| **BUTTON_CFG_ASSERT_EN** 		| Enable/Disable assertions. |
| **BUTTON_PRINT** 			    | Definition of debug printing. |
| **BUTTON_ASSERT** 			| Definition of assert. |


**4. Set up configuration table inside **button_cfg.c** file:**
```C
/**
 *     Button configuration table
 *
 *  @brief  LPF is RC 1st order IIR filter. Output is being compared between
 *          5% and 95% (3 Tao).
 *
 *          E.g. LPF with fc=1Hz will result in button detection time delay
 *          of:
 *
 *                 fc = 1 / ( 2*pi*Tao ) ---> Tao = 1 / ( 2*pi*fc )
 *
 *                 dt = 3 Tao = 3 * 1 / ( 2*pi*fc )
 *
 *                 fc = 1.0 Hz ---> dt = 0.477 sec
 */
static const button_cfg_t g_button_cfg[ eBUTTON_NUM_OF ] =
{
    // USER CODE BEGIN...

    // -----------------------------------------------------------------------------------------------------------------------------------------------------------------
    //                              Gpio pin enumeration                        Button polarity                         LPF enable          Default LPF fc
    // -----------------------------------------------------------------------------------------------------------------------------------------------------------------
    [eBUTTON_BUCKET_HOME_SW]    = {	.gpio_pin = eGPIO_END_SW_BUCKET_HOME,   	.polarity = eBUTTON_POL_ACTIVE_HIGH,	.lpf_en = true,    .lpf_fc = 10.0f          },
    [eBUTTON_DELIVERY_OPEN_SW]  = {	.gpio_pin = eGPIO_END_SW_DELIVERY_OPEN, 	.polarity = eBUTTON_POL_ACTIVE_HIGH,	.lpf_en = true,    .lpf_fc = 10.0f          },
    [eBUTTON_DELIVERY_CLOSE_SW] = {	.gpio_pin = eGPIO_END_SW_DELIVERY_CLOSE,	.polarity = eBUTTON_POL_ACTIVE_HIGH,	.lpf_en = true,    .lpf_fc = 10.0f          },
    [eBUTTON_DELIVERY_FAULT]    = {	.gpio_pin = eGPIO_DELIVERY_BRIDGE_FAULT,	.polarity = eBUTTON_POL_ACTIVE_HIGH,	.lpf_en = true,    .lpf_fc = 10.0f          },
    [eBUTTON_SLIDER_FAULT]      = {	.gpio_pin = eGPIO_SLIDER_BRIDGE_FAULT,  	.polarity = eBUTTON_POL_ACTIVE_HIGH,	.lpf_en = true,    .lpf_fc = 10.0f          },

    // USER CODE END...
};
```

**5. Include, initialize & handle:**

Main button handler **button_hndl()** must be called with a fixed period of **BUTTON_CFG_HNDL_PERIOD_S** (defined inside button_cfg.h). 

**NOTICE: Before using Button module GPIO shall be initialized!**

```C
// Include single file to your application!
#include "button.h"

// Init button
if ( eBUTTON_OK != button_init())
{
    // Init failed...
    // Further actions here...
}

/**
 * @note    Make sure to have a fixed period of BUTTON_CFG_HNDL_PERIOD_S in order
 *          to get equidistant sampling for filtering purposes
 */ 
@BUTTON_CFG_HNDL_PERIOD_S period
{
    // Handle button
    button_hndl();

    // Get button state
    button_get_state( eBUTTON_USER, &my_button_state );

    if ( eBUTTON_ON == my_button_state )
    {
        // Buttton is pressed actions here...

        // Get button press time
        button_get_time( eBUTTON_USER, &active_time, NULL );

        // Is long press
        if ( active_time > LONG_BUTTON_PRESS_TIME )
        {
            // Button is pressed for "long" period of time...
        }
    }
}
```

**6. Register and use callback functions**
```C
// Declare callback functions
static void my_button_pressed(void);
static void my_button_released(void);

// Register callback functions
button_register_callback( eBUTTON_USER, &my_button_pressed, &my_button_released );

// Define callback functions
static void my_button_pressed(void)
{
    // Put actions on button pressed event here...
}

static void my_button_released(void)
{
    // Put actions on button release event here...
}
```
