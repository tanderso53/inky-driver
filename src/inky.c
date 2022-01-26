#include "luts.h"
#include <inky-api.h>

#include <stdlib.h>

/*
**********************************************************************
*********************** Library Definitions **************************
**********************************************************************
*/

/** @brief Macro to check and return result of function on failure */
#define INKY_CHECK_RESULT(retvalue, result) do {	\
		if (retvalue != result) {		\
			return retvalue;		\
		}					\
	} while (0)

/* Commands recognized by peripheral */
typedef enum {
	SOFT_RESET		= 0x12, /* Soft Reset */
	ANALOG_BLOCK_CONTROL	= 0x74, /* Set Analog Block Control */
	DIGITAL_BLOCK_CONTROL	= 0x7e, /* Set Digital Block Control */
	GATE_SETTING		= 0x01, /* Gate setting */
	GATE_DRIVING_VOLTAGE	= 0x03, /* Gate Driving Voltage */
	SOURCE_DRIVING_VOLTAGE	= 0x04, /* Source Driving Voltage */
	DUMMY_LINE_PERIOD	= 0x3a, /* Dummy line period */
	GATE_LINE_WIDTH		= 0x3b, /* Gate line width */
	DATA_ENTRY_MODE		= 0x11, /* Data entry mode setting 0x03 = X/Y increment */
	VCOM_REGISTER		= 0x2c, /* VCOM Register, 0x3c = -1.5v? */
	GS_TRANSITION_DEFINE	= 0x3c, /* GS Transition Define A + VSS + LUT0 */
	SET_LUTS		= 0x32, /* Set LUTs */
	RAM_X_RANGE		= 0x44, /* Set RAM X Start/End */
	RAM_Y_RANGE		= 0x45, /* Set RAM Y Start/End */
	RAM_X_PTR_START		= 0x4e, /* Set RAM X Pointer Start */
	RAM_Y_PTR_START		= 0x4f, /* Set RAM Y Pointer Start */
	WRITE_PIXEL_BLACK	= 0x24, /* Write pixel black/white */
	WRITE_PIXEL_COLOR	= 0X26, /* Write pixel yellow or red */
	UPDATE_SEQUENCE		= 0x22, /* Display Update Sequence */
	TRIGGER_UPDATE		= 0x20, /* Trigger Display Update */
	ENTER_DEEP_SLEEP	= 0x10 /* Enter Deep Sleep */
} dcommand;

static struct _display_res {
	UINT16_t height;
	UINT16_t width;
} _inky_what = {
	.height = 300,
	.width = 400
}, _inky_phat = {
	.height = 250,
	.width = 122
};

/** @brief Send data on SPI bus
 * @p data Data to send on bus
 */
static inky_error_state _spi_send_data(inky_config *cfg,
				       const UINT8_t *data,
				       UINT32_t len);

/** @brief send command with optional data through spi bus
 * @p command selected from dcommand
 * @p data data is optional and is ignored if NULL is passed
 */
static inky_error_state _spi_send_command(inky_config *cfg,
					  dcommand cmd,
					  const UINT8_t *data,
					  UINT32_t len);

static inky_error_state _spi_send_command_byte(inky_config *cfg,
					       dcommand cmd,
					       UINT8_t arg);

static inky_error_state _reset(inky_config *cfg);

static inky_error_state _busy_wait(inky_config *cfg);

static inky_error_state _allocate_fb(inky_config *cfg);

static UINT8_t* _spi_order_bytes(UINT16_t input, UINT8_t* result);

static inky_error_state _inky_prep(inky_config *cfg,
				   UINT8_t *height_byte_array);

/*
**********************************************************************
********************** Driver Implementation *************************
**********************************************************************
*/

inky_error_state inky_setup(inky_config *cfg)
{
	inky_error_state ret;

	if ((ret = cfg->gpio_init_cb()) != OK) {
		return ret;
	}

	if ((ret = cfg->gpio_setup_pin_cb(DC_PIN, OUT, LOW, OFF)) != OK) {
		return ret;
	}

	if ((ret = cfg->gpio_setup_pin_cb(RESET_PIN, OUT, HIGH, OFF)) != OK) {
		return ret;
	}

	if ((ret = cfg->gpio_setup_pin_cb(BUSY_PIN, IN, INPUT, OFF)) != OK) {
		return ret;
	}

	if ((ret = cfg->spi_setup_cb()) != OK) {
		return ret;
	}

	if ((cfg->exclude_flags & INKY_FLAG_ALLOCATE_FB) == 0) {
		if ((ret = _allocate_fb(cfg)) != OK) {
			return ret;
		}
	}

	/* _reset will block until polling BUSY_PIN returns, or will
	   time out */
	if ((ret = _reset(cfg)) != 0) {
		return ret;
	}

	return OK;
}

inky_error_state inky_free(inky_config *cfg)
{
	if (cfg->fb) {
		free(cfg->fb->buffer);
		free(cfg->fb);
	}

	if (cfg->active_fb) {
		free(cfg->active_fb->buffer);
		free(cfg->active_fb);
	}

	return OK;
}

inky_error_state inky_fb_usrptr_attach(inky_config *cfg, UINT8_t pos,
				       void *ptr)
{
	if (!cfg->fb) {
		return NOT_CONFIGURED;
	}

	switch (pos) {
	case 1:
		cfg->fb->usrptr1 = ptr;
		break;
	case 2:
		cfg->fb->usrptr2 = ptr;
		break;
	default:
		return NOT_AVAILABLE;
		break;
	}

	return OK;
}

inky_error_state inky_fb_set_pixel(inky_config *cfg, UINT8_t x,
				   UINT8_t y, inky_color c)
{
	if (!cfg->fb) {
		return NOT_CONFIGURED;
	}

	if (x >= cfg->fb->width) {
		return OUT_OF_RANGE;
	}

	if (y >= cfg->fb->height) {
		return OUT_OF_RANGE;
	}

	switch (c) {
	case BLACK:
		if (cfg->color->black == 0) {
			return NOT_AVAILABLE;
		}
		break;
	case WHITE:
		if (cfg->color->white == 0) {
			return NOT_AVAILABLE;
		}
		break;
	case RED:
		if (cfg->color->red == 0) {
			return NOT_AVAILABLE;
		}
		break;
	case YELLOW:
		if (cfg->color->yellow == 0) {
			return NOT_AVAILABLE;
		}
		break;
	default:
		return NOT_AVAILABLE;
	}

	cfg->fb->buffer[cfg->fb->width * y + x] = (UINT8_t) c;

	return OK;
}

inky_error_state inky_update(inky_config *cfg)
{
	inky_error_state ret;
	UINT8_t height_byte_array[2];

	ret = _inky_prep(cfg, height_byte_array);

	if (ret != OK) {
		return ret;
	}

	/* Set ram X and Y  start and end */
	ret = _spi_send_command(cfg, RAM_X_RANGE,
				(UINT8_t[]) {0x00, (cfg->fb->width / 8) - 1},
				2);
	INKY_CHECK_RESULT(ret, OK);

	ret = _spi_send_command(cfg, RAM_Y_RANGE,
				(UINT8_t[]) {0x00, 0x00,
					     height_byte_array[1],
					     height_byte_array[0]}, 4);
	INKY_CHECK_RESULT(ret, OK);

	/* Write the framebuffer to the display */
	for (UINT16_t i = 0; i < cfg->fb->height; i++) {
		UINT32_t arr_addr = i * cfg->fb->width;
		UINT8_t row[cfg->fb->width];
		UINT8_t row_color[cfg->fb->width];
		UINT8_t sweep_color = 0;
		UINT8_t row_addr[2];

		/* Write black and color rows to separate arrays */
		for (UINT16_t j = 0; j < cfg->fb->width; j++) {
			row[j] = cfg->fb->buffer[arr_addr] == 1 ? 1 : 0;
			row_color[j] = cfg->fb->buffer[arr_addr] > 1 ? 1 : 0;

			if (row_color[j] > 0)
				sweep_color = 1;

			arr_addr++;
		}

		if (!_spi_order_bytes(i, row_addr))
			return NULL_PTR;

		ret = _spi_send_command_byte(cfg, RAM_X_PTR_START, 0x00);
		INKY_CHECK_RESULT(ret, OK);

		ret = _spi_send_command(cfg, RAM_Y_PTR_START, row_addr, 2);
		INKY_CHECK_RESULT(ret, OK);

		/* Write black/white row */
		ret = _spi_send_command(cfg, WRITE_PIXEL_BLACK, row,
					cfg->fb->width);
		INKY_CHECK_RESULT(ret, OK);

		/* Write color row if it exists */
		if (sweep_color) {
			_spi_send_command_byte(cfg, RAM_X_PTR_START, 0x00);
			_spi_send_command(cfg, RAM_Y_PTR_START, row_addr, 2);

			_spi_send_command(cfg, WRITE_PIXEL_COLOR, row,
					  cfg->fb->width);
		}
	}

	/* Trigger the refresh and write operation on display */
	ret = _spi_send_command_byte(cfg, UPDATE_SEQUENCE, 0xC7);
	INKY_CHECK_RESULT(ret, OK);

	ret = _spi_send_command(cfg, TRIGGER_UPDATE, NULL, 0);
	INKY_CHECK_RESULT(ret, OK);

	ret = cfg->delay_us_cb(50);
	INKY_CHECK_RESULT(ret, OK);

	ret = _busy_wait(cfg);
	INKY_CHECK_RESULT(ret, OK);

	/* Put display to sleep */
	ret = _spi_send_command_byte(cfg, ENTER_DEEP_SLEEP, 0x01);
	INKY_CHECK_RESULT(ret, OK);

	return OK;
}

inky_error_state inky_update_by_mode(inky_config *cfg,
				     inky_fb_type update_type)
{
	/* TODO: Implement function to update, overriding configure
	 * mode */
}

/*
**********************************************************************
************************* INTERNAL API *******************************
**********************************************************************
*/

static inky_error_state _spi_send_data(inky_config *cfg,
				       const UINT8_t *data,
				       UINT32_t len)
{
	return cfg->spi_write_cb(data, len);
}

static inky_error_state _spi_send_command(inky_config *cfg, dcommand cmd,
					  const UINT8_t *data, UINT32_t len)
{
	inky_error_state ret;

	ret = cfg->spi_write_cb((UINT8_t*) &cmd, 1);

	if (ret != OK) {
		return ret;
	}

	if (data) {
		ret = _spi_send_data(cfg, data, len);
		return ret;
	}

	return OK;
}

static inky_error_state _spi_send_command_byte(inky_config *cfg,
					       dcommand cmd,
					       UINT8_t arg)
{
	inky_error_state ret;

	ret = cfg->spi_write_cb((UINT8_t*) &cmd, 1);

	if (ret != OK) {
		return ret;
	}

	ret = _spi_send_data(cfg, &arg, 1);
	return ret;
}

static inky_error_state _reset(inky_config *cfg)
{
	inky_error_state ret;

	if (!cfg->fb) {
		/* Stop if not configured */
		return NOT_CONFIGURED;
	}

	ret = cfg->gpio_output_cb(RESET_PIN, LOW);

	if (ret != OK) {
		return ret;
	}

	cfg->delay_us_cb(100000);
	cfg->gpio_output_cb(RESET_PIN, HIGH);

	if (ret != OK) {
		return ret;
	}

	cfg->delay_us_cb(100000);

	/* Send soft reset */
	if ((ret =_spi_send_command(cfg, SOFT_RESET, NULL, 0)) != OK) {
		return ret;
	}

	/* Poll the busy pin, blocking until it returns */
	if ((ret = _busy_wait(cfg)) != OK) {
		return ret;
	}

	return OK;
}

static inky_error_state _busy_wait(inky_config *cfg)
{
	return cfg->gpio_poll_cb(BUSY_PIN, 30000);
}

static inky_error_state _allocate_fb(inky_config *cfg)
{
	struct _display_res *dr;

	if (cfg->fb) {
		return OK;
	}

	switch (cfg->pdt) {
	case INKY_WHAT:
		dr = &_inky_what;
		break;
	case INKY_PHAT:
		dr = &_inky_phat;
		break;
	default:
		return NOT_AVAILABLE;
		break;
	}

	cfg->fb = malloc(sizeof(inky_fb));

	if (!cfg->fb) {
		return OUT_OF_MEMORY;
	}

	cfg->fb->buffer = malloc(dr->height * dr->width * sizeof(UINT8_t));

	if (!cfg->fb->buffer) {
		return OUT_OF_MEMORY;
	}

	cfg->fb->width = dr->width;
	cfg->fb->height = dr->height;

	if ((cfg->exclude_flags & INKY_FLAG_REFRESH_ALWAYS & INKY_FLAG_NO_DIFF) != 0) {
		cfg->fb->fb_type = REFRESH_DIFF;
	} else if ((cfg->exclude_flags & INKY_FLAG_REFRESH_ALWAYS) != 0) {
		cfg->fb->fb_type = OVERLAY;
	} else {
		cfg->fb->fb_type = REFRESH_ALWAYS;
	}

	cfg->active_fb = NULL;

	return OK;
}

static UINT8_t* _spi_order_bytes(UINT16_t input, UINT8_t* result) {
	UINT8_t height_byte_little;
	UINT8_t height_byte_big;

	if (!result) {
		return NULL;
	}

	/* Fit 16bit heights into 8bit message stream */
	height_byte_little = (UINT8_t) (input & 0x00FF);
	height_byte_big = (UINT8_t) ((input >> 2) & 0x00FF);

	result[0] = height_byte_big;
	result[1] = height_byte_little;

	return result;
}

static inky_error_state _inky_prep(inky_config *cfg, 
				   UINT8_t *height_byte_array)
{
	inky_error_state ret;

	if ((ret = _reset(cfg))) {
		return ret;
	}

	if (!_spi_order_bytes(cfg->fb->height, height_byte_array))
		return NULL_PTR;

	/* Use command sequence from Pimoroni's Inky library */
	_spi_send_command_byte(cfg, ANALOG_BLOCK_CONTROL, 0x54);
	_spi_send_command_byte(cfg, DIGITAL_BLOCK_CONTROL, 0x3b);
	_spi_send_command(cfg, GATE_SETTING, height_byte_array, 3);
	_spi_send_command_byte(cfg, GATE_DRIVING_VOLTAGE, 0x17);
	_spi_send_command(cfg, SOURCE_DRIVING_VOLTAGE,
			  (UINT8_t[]) {0x41, 0xac, 0x32}, 3);
	_spi_send_command_byte(cfg, DUMMY_LINE_PERIOD, 0x07);
	_spi_send_command_byte(cfg, GATE_LINE_WIDTH, 0x04);
	_spi_send_command_byte(cfg, DATA_ENTRY_MODE, 0x03);
	_spi_send_command_byte(cfg, VCOM_REGISTER, 0x3c);

	/* Set border config to white */
	_spi_send_command_byte(cfg, GS_TRANSITION_DEFINE, 0x00);
	_spi_send_command_byte(cfg, GS_TRANSITION_DEFINE, 0x31);

	if (cfg->color->yellow) {
		_spi_send_command(cfg, SOURCE_DRIVING_VOLTAGE,
				  (UINT8_t[]) {0x07, 0xac, 0x32},
				  3);
	}

	if (cfg->color->red && (cfg->pdt == INKY_WHAT)) {
		_spi_send_command(cfg, SOURCE_DRIVING_VOLTAGE,
				  (UINT8_t[]) {0x30, 0xac, 0x22},
				  3);
	}

	/* Support for different update modes will be added later */
	switch (cfg->fb->fb_type) {
	case REFRESH_ALWAYS:
		if (cfg->color->yellow) {
			_spi_send_command(cfg, SET_LUTS,
					  lut_yellow_refresh,
					  sizeof(lut_yellow_refresh));
		} else if (cfg->color->red) {
			_spi_send_command(cfg, SET_LUTS,
					  lut_red_refresh,
					  sizeof(lut_red_refresh));
		}
		else {
			_spi_send_command(cfg, SET_LUTS,
					  lut_black_refresh,
					  sizeof(lut_black_refresh));
		}

		break;
	default:
		return NOT_AVAILABLE;
		break;
	}

	return OK;
}
