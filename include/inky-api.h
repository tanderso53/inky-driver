#ifndef INKY_API_H
#define INKY_API_H

#ifndef DISABLE_POSIX
#include <stdint.h>
#endif /* #ifndef DISABLE_POSIX */

/*
**********************************************************************
************************* API Definitions ****************************
**********************************************************************
*/

/**
 * @defgroup inkydisplayapi INKY Display API
 * @{
 */

/**
 * @defgroup inkyintoverides Integer overides
 * @{
 */
#ifndef INT8_t
#define INT8_t int8_t
#endif

#ifndef UINT8_t
#define UINT8_t uint8_t
#endif

#ifndef INT16_t
#define INT16_t int16_t
#endif

#ifndef UINT16_t
#define UINT16_t uint16_t
#endif

#ifndef INT32_t
#define INT32_t int32_t
#endif

#ifndef UINT32_t
#define UINT32_t uint32_t
#endif

#ifndef UINT64_t
#define UINT64_t uint64_t
#endif

/**
 * @}
 * Intgeger overides
 */

/**
 * @defgroup inkysetupflags Inky Setup Flags
 * @{
 */

/* Setup flags, on by default */
#define INKY_FLAG_MAP_COLORS		0x0001
#define INKY_FLAG_ALLOCATE_FB		0x0002
#define INKY_FLAG_REFRESH_ALWAYS	0x0004
#define INKY_FLAG_NO_DIFF		0x0008

#define INKY_SPI_SPEED_HZ_MAX		488000
#define INKY_SPI_BITS_DEFAULT		8

/**
 * @}
 * Inky Setup Flags
 */

/** @defgroup inkyerrorstates Inky error states
 * @{
 */

#define INKY_OK				 0
#define INKY_E_NOT_AVAILABLE		-1
#define INKY_E_TIMEOUT			-2
#define INKY_E_BAD_PERMISSIONS		-3
#define INKY_E_OUT_OF_MEMORY		-4
#define INKY_E_OUT_OF_RANGE		-5
#define INKY_E_NOT_CONFIGURED		-6
#define INKY_E_NULL_PTR			-7
#define INKY_E_FAILURE			-8
#define INKY_E_COMM_FAILURE		-9

/**
 * @}
 */

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
 * @defgroup inkygpiocfg GPIO pin configuration enums
 * @{
 */
	typedef enum {
		/* GPIO Pins */
		INKY_PIN_RESET,
		INKY_PIN_BUSY,
		INKY_PIN_DC,
		/* In addition the following pins are used for SPI */
		INKY_PIN_CS,
		INKY_PIN_MOSI,
		INKY_PIN_SCLK
	} inky_pin;

	typedef enum {
		INKY_DIR_IN,
		INKY_DIR_OUT
	} inky_gpio_direction;

	typedef enum {
		INKY_PINCFG_OFF = 0,
		INKY_PINCFG_PULLUP,
		INKY_PINCFG_PULLDOWN
	} inky_gpio_pull_up_down;

	typedef enum {
		INKY_PINSTATE_LOW = 0,
		INKY_PINSTATE_HIGH = 1,
		INKY_PINSTATE_INPUT
	} inky_pin_state;

/**
 * @}
 * GPIO pin configuration enums
 */

/**
 * @defgroup inkycbtypes Types for user callback functions
 * @{
 */

	typedef int8_t inky_error_state;

/* GPIO function pointer types to be set by user */
	typedef inky_error_state (*inky_user_gpio_initialize)(void*);
	typedef inky_error_state (*inky_user_gpio_setup_pin)(inky_pin,
							     inky_gpio_direction,
							     inky_pin_state,
							     inky_gpio_pull_up_down,
							     void*);
	typedef inky_error_state (*inky_user_gpio_output_state)(inky_pin,
								inky_pin_state,
								void*);
	typedef inky_error_state (*inky_user_gpio_input_state)(inky_pin,
							       inky_pin_state*,
							       void*);
	typedef inky_error_state (*inky_user_gpio_poll_pin)(inky_pin,
							    UINT64_t,
							    void*);

/* SPI function pointer types to be set by user */
	typedef inky_error_state (*inky_user_spi_setup)(void*);

/* Typical kernel callbacks */
	typedef inky_error_state (*inky_user_delay)(UINT32_t, void*);

/** @brief inky_user_spi_write
 *  @param buf ptr to buffer to write
 *  @param len length of buffer to write
 */
	typedef inky_error_state (*inky_user_spi_write)(const UINT8_t*,
							UINT32_t,
							void*);

/** @brief inky_user_spi_write_16
 *  @param buf ptr to buffer to write
 *  @param len length of buffer to write
 */
	typedef inky_error_state (*inky_user_spi_write_16)(const UINT16_t*,
							   UINT32_t,
							   void*);

/**
 * @}
 * Types for user callback functiosn
 */

/** @brief Inky definitions */
	typedef enum {
		INKY_WHAT,
		INKY_PHAT,
		INKY_CUSTOM
	} inky_product;

/** @brief Colors various inkies can handle */
	typedef enum {
		INKY_COLOR_WHITE = 0,
		INKY_COLOR_BLACK = 1,
		INKY_COLOR_RED = 2,
		INKY_COLOR_YELLOW = 3
	} inky_color;

/** @brief Define the available colors in this struct, 1 on, 0 off */
	typedef struct inky_color_confignode {
		UINT8_t white;
		UINT8_t black;
		UINT8_t red;
		UINT8_t yellow;
	} inky_color_config;

/* Framebuffer Definitions */

/** @brief Framebuffer type dictates how the buffer is displayed and
 * refreshed
 * @var INKY_FB_REFRESH_ALWAYS full refresh anytime the fb is written
 * @var INKY_FB_REFRESH_DIFF only refresh regions that change
 * @var INKY_FB_OVERLAY Manual refresh only (for terminals)
 */
	typedef enum {
		INKY_FB_REFRESH_ALWAYS, 
		INKY_FB_REFRESH_DIFF,
		INKY_FB_OVERLAY,
	} inky_fb_type;

/** @brief Framebuffer is defined by the following struct, but will be
 *  setup by the API commands in this section unless the user decides
 *  they are unworthy of use */
	typedef struct inky_fbnode {
		UINT16_t width;
		UINT16_t height;
		UINT8_t *buffer;
		UINT16_t bytes;
		inky_fb_type fb_type;
		void *usrptr1;
		void *usrptr2;
	} inky_fb;

	typedef UINT16_t inky_flags;

/** @brief Configuration structure for Inky, to pass to setup function
    @var pdt INKY_WHAT, INKY_PHATT, etc
    @var *color Available Colors
    @var *fb Frame buffer to be allocated by library (note: must free later)
    @var *active_fb Not set by user. Always null when REFRESH_ALWAYS
    @var exclude_flags Config flags to remove
    @var gpio_init_cb gpio init callback
**/
	typedef struct inky_confignode {
		inky_product pdt;
		inky_color_config *color;
		inky_fb *fb;
		inky_fb *active_fb;
		inky_flags exclude_flags;
		inky_user_gpio_initialize gpio_init_cb;
		inky_user_gpio_setup_pin gpio_setup_pin_cb; /**< GPIO pin config callback */
		inky_user_gpio_output_state gpio_output_cb; /**< GPIO set output callback */
		inky_user_gpio_input_state gpio_input_cb; /**< GPIO set input callback */
		inky_user_gpio_poll_pin gpio_poll_cb; /**< Callback to wait for pin */
		inky_user_spi_setup spi_setup_cb; /**< SPI setup callback */
		inky_user_spi_write spi_write_cb; /**< SPI 8 bit array write callback */
		inky_user_spi_write_16 spi_write16_cb; /**< SPI 16 bit array write callback */
		inky_user_delay delay_us_cb; /**< Delay callback with time in us */
		void *intf_ptr; /**< Pointer user interface object */
		void *usrptr1; /**< Optional usrptr. Pass NULL if not needed */
		void *usrptr2; /**< Optional usrptr. Pass NULL if not needed */
	} inky_config;

/** @brief Setup Function */
	inky_error_state inky_setup(inky_config *cfg);

/** @brief Free memory from setup */
	inky_error_state inky_free(inky_config *cfg);

/** @brief Attach usrptr to framebuffer */
	inky_error_state inky_fb_usrptr_attach(inky_config *cfg,
					       UINT8_t pos, void *ptr);

/** @brief Set pixel color in fb */
	inky_error_state inky_fb_set_pixel(inky_config *cfg, UINT16_t x,
					   UINT16_t y, inky_color c);

/** @brief Update Inky screen to current fb state using config update
 * mode */
	inky_error_state inky_update(inky_config *cfg);

/** @brief Update Inky screen to current fb by given mode */
	inky_error_state inky_update_by_mode(inky_config *cfg,
					     inky_fb_type update_type);

/** @brief Clear Inky screen */
	inky_error_state inky_clear(inky_config *cfg);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

/**
 * @}
 * INKY Display API
 */

#endif /* #ifndef INKY_API_H */
