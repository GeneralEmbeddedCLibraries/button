# **Button library**
General C library for button action handling. Library is simple to use as it is configurable by single table. It also features debouncing filtering. 


## **Dependencies**

### **1. GPIO module**
--- 
Button library is dependend from low level GPIO module thus following function prototypes must be provided:
  - gpio_status_t gpio_is_init(bool * const p_is_init)
  - void gpio_set(const gpio_pins_t pin, const gpio_state_t state)

GPIO translation unit must be under following project path:
```C
#include "drivers/peripheral/gpio/gpio.h"
```

### **2. Filter module**
--- 
Button library is also dependent from filter module. Filter module sources can be found under this [link](https://github.com/GeneralEmbeddedCLibraries/filter). Filter module must take following path:

```C
#include "middleware/filter/src/filter.h"
```

### **3. float32_t definition**
Definition of float32_t must be provided by user. In current implementation it is defined in "project_config.h". Just add following statement to your code where it suits the best.

```C
// Define float
typedef float float32_t;
```
### **4. Static assert**
Additionaly module uses "static_assert()" function defined in <assert.h>. It is being used for cross-module compatibility.

 ## **API**
---
| API Functions | Description | Prototype |
| --- | ----------- | ----- |
| **button_init** | Initialization of button module | button_status_t button_init(void) |****
| **button_is_init** | Get initialization flag | button_status_t 	button_is_init(bool * const p_is_init) |
| **button_hndl** | Main button handler | button_status_t button_hndl(void) |
| **button_get_state** | Get button state | button_status_t button_get_state            (const button_num_t num, bool * const p_state) |
| **button_change_filter_fc** | Change button cutoff frequency of filter | button_status_t button_change_filter_fc		(const button_num_t num, const float32_t fc) |
| **button_get_time** | Get button timings | button_status_t button_get_time				(const button_num_t num, float32_t * const p_active_time, float32_t * const p_idle_time)|
| **button_register_callback** | Register button callback | button_status_t button_register_callback	(const button_num_t num, pf_button_callback pf_pressed, pf_button_callback pf_released) |
| **button_unregister_callback** | Un-register button callback | button_status_t button_unregister_callback	(const button_num_t num) |




## **How to use**
---

1. List all buttons inside **button_cfg.h** file:
```C
/**
 * 	List of Buttons
 *
 * @note 	User shall provide Button name here as it would be using
 * 			later inside code.
 *
 * 			Shall always start with 0.
 *
 * @note 	User shall change code only inside section of "USER_CODE_BEGIN"
 * 			ans "USER_CODE_END".
 */
typedef enum
{
	// USER CODE START...

	eBUTTON_USER = 0,
	eBUTTON_RE,


	// USER CODE END...

	eBUTTON_NUM_OF
} button_num_t;
```

2. Set up main handler period inside **button_cfg.h** file:
```C
/**
 * 	Main button handler period
 * 	Unit: sec
 */
#define BUTTON_CFG_HNDL_PERIOD_S					( 0.01f )
```

3. Set up configuration table inside **button_cfg.c** file:
```C
/**
 * 	Button configuration table
 *
 * 	@brief	LPF is RC 1st order IIR filter. Output is being compared between
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
 * 	@note 	Low level gpio code must be compatible!
 */
static const button_cfg_t g_button_cfg[ eBUTTON_NUM_OF ] =
{

	// USER CODE BEGIN...

	// -------------------------------------------------------------------------------------------------------------------------
	//	Gpio pin						Button polarity							LPF enable			Default LPF fc
	// -------------------------------------------------------------------------------------------------------------------------
	{ 	.gpio_pin = eGPIO_USER_BTN,		.polarity = eBUTTON_POL_ACTIVE_LOW,		.lpf_en = false, 	.lpf_fc = 1.0f 				},
	{ 	.gpio_pin = eGPIO_RE_BTN,		.polarity = eBUTTON_POL_ACTIVE_LOW,		.lpf_en = false, 	.lpf_fc = 1.0f 				},


	// USER CODE END...

};
```

4. Include, initialize & handle:

Main button handler **button_hndl()** must be called with a fixed period of **BUTTON_CFG_HNDL_PERIOD_S** (defined inside button_cfg.h). 

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

5. Register and use callback functions
```C
// Declare callback functions
static void my_button_pressed(void);
static void my_button_released(void);

// Register callback functions
button_register_callback( eBUTTON_USER, &my_button_pressed, &my_button_released );

// Define callback functions
static void my_button_pressed(void)
{
    // Put actions on button release event here...
}

static void my_button_released(void)
{
    // Put actions on button release event here...
}

```
