#ifndef INKY_API_H
#define INKY_API_H

#ifndef DISABLE_POSIX
#include <stdint.h>
#endif /* #ifndef DISABLE_POSIX */

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/*
**********************************************************************
************************* API Definitions ****************************
**********************************************************************
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

/* GPIO pins required by BCM number */
typedef enum {
	/* GPIO Pins */
	RESET_PIN,
	BUSY_PIN,
	DC_PIN,
	/* In addition the following pins are used for SPI */
	CS_PIN,
	MOSI_PIN,
	SCLK_PIN
} inky_pin;

typedef enum {
	IN,
	OUT
} inky_gpio_direction;

typedef enum {
	OFF = 0,
	PULLUP,
	PULLDOWN
} inky_gpio_pull_up_down;

typedef enum {
	LOW = 0,
	HIGH = 1
} inky_pin_state;

typedef enum {
	OK = 0,
	NOT_AVAILABLE,
	TIMED_OUT,
	BAD_PERMISSIONS,
	OUT_OF_MEMORY,
	NOT_CONFIGURED,
	FAILURE = 1
} inky_error_state;

/* GPIO function pointer types to be set by user */
typedef inky_error_state (*inky_user_gpio_initialize)();
typedef inky_error_state (*inky_user_gpio_setup_pin)(inky_pin,
						     inky_gpio_direction,
						     inky_pin_state,
						     inky_gpio_pull_up_down);
typedef inky_error_state (*inky_user_gpio_output_state)(inky_pin, inky_pin_state);
typedef inky_error_state (*inky_user_gpio_input_state)(inky_pin, inky_pin_state*);

/* SPI function pointer types to be set by user */
typedef inky_error_state (*inky_user_spi_setup)();

/* inky_user_spi_write
   :param UINT8_t buf: ptr to buffer to write
   :param UINT32_t len: length of buffer to write
*/
typedef inky_error_state (*inky_user_spi_write)(UINT8_t*, UINT32_t);

/* inky_user_spi_write_16
   :param UINT16_t buf: ptr to buffer to write
   :param UINT32_t len: length of buffer to write
*/
typedef inky_error_state (*inky_user_spi_write_16)(UINT16_t*, UINT32_t);

/* Inky definitions */
typedef enum {
	INKY_WHAT,
	INKY_PHAT,
	INKY_CUSTOM
} inky_product;

/* Colors various inkies can handle */
typedef enum {
	WHITE = 0,
	BLACK = 1,
	RED,
	YELLOW
} inky_color;

/* Define the available colors in this struct, 1 on, 0 off */
typedef struct inky_color_confignode {
	UINT8_t white;
	UINT8_t black;
	UINT8_t red;
	UINT8_t yellow;
} inky_color_config;

/* Framebuffer Definitions */

/* Framebuffer type dictates how the buffer is displayed and
 * refreshed */
typedef enum {
	REFRESH_ALWAYS, /* full refresh anytime the fb is written */
	REFRESH_DIFF, /* only refresh regions that change */
	OVERLAY, /* Manual refresh only (for terminals) */
} inky_fb_type;

/* Framebuffer is defined by the following struct, but will be setup
   by the API commands in this section unless the user decides they
   are unworthy of use */
typedef struct inky_fbnode {
	UINT16_t width;
	UINT16_t height;
	UINT8_t *buffer;
	inky_fb_type fb_type;
	void *usrptr1;
	void *usrptr2;
} inky_fb;

typedef UINT32_t inky_flags;

/* Configuration structure for Inky, to pass to setup function */
typedef struct inky_confignode {
	inky_product pdt; /* INKY_WHAT, INKY_PHATT, etc */
	inky_color_config *color; /* Available Colors */
	inky_fb *fb; /* Frame buffer to be allocated by library (note: must free later) */
	inky_fb *active_fb; /* Not set by user. Always null when REFRESH_ALWAYS */
	inky_flags flags; /* Extra config flags from defines */
	inky_user_gpio_initialize gpio_init_cb; /* gpio init callback */
	inky_user_gpio_setup_pin gpio_setup_pin_cb; /* GPIO pin config callback */
	inky_user_gpio_output_state gpio_output_cb; /* GPIO set output callback */
	inky_user_gpio_input_state gpio_input_cb; /* GPIO set input callback */
	inky_user_spi_setup spi_setup_cb; /* SPI setup callback */
	inky_user_spi_write spi_write_cb; /* SPI 8 bit array write callback */
	inky_user_spi_write_16 spi_write16_cb; /* SPI 16 bit array write callback */
	void *usrptr1; /* Optional usrptr. Pass void if not needed */
	void *usrptr2; /* Optional usrptr. Pass void if not needed */
} inky_config;

/* Setup Function */
inky_error_state inky_setup(inky_config *cfg);

/* Free memory from setup */
inky_error_state inky_free(inky_config *cfg);

/* Attach usrptr to framebuffer */
inky_error_state inky_fb_usrptr_attach(UINT8_t pos, void *ptr);

/* Set pixel color in fb */
inky_error_state inky_fb_set_pixel(inky_config *cfg, UINT8_t x,
				   UINT8_t y, inky_color c);

/* Update Inky screen to current fb state using config udate mode */
inky_error_state inky_update(inky_config *cfg);

/* Update Inky screen to current fb by given mode */
inky_error_state inky_update_by_mode(inky_config *cfg,
				     inky_fb_type update_type);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */
#endif /* #ifndef INKY_API_H */
