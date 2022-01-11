#include "luts.h"
#include <inky-api.h>

#include <stdlib.h>

/*
**********************************************************************
*********************** Library Definitions **************************
**********************************************************************
*/

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
				       UINT8_t *data,
				       UINT32_t len);

/** @brief send command with optional data through spi bus
 * @p command selected from dcommand
 * @p data data is optional and is ignored if NULL is passed
 */
static inky_error_state _spi_send_command(inky_config *cfg,
					  dcommand cmd,
					  UINT8_t *data,
					  UINT32_t len);

static inky_error_state _reset(inky_config *cfg);

static inky_error_state _busy_wait(inky_config *cfg);

static inky_error_state _allocate_fb(inky_config *cfg);

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
}

inky_error_state inky_update_by_mode(inky_config *cfg,
				     inky_fb_type update_type)
{
}

static inky_error_state _spi_send_data(inky_config *cfg,
				       UINT8_t *data,
				       UINT32_t len)
{
	return cfg->spi_write_cb(data, len);
}

static inky_error_state _spi_send_command(inky_config *cfg,
					  dcommand cmd, UINT8_t *data,
					  UINT32_t len)
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

	cfg->delay_ms_cb(100);
	cfg->gpio_output_cb(RESET_PIN, HIGH);

	if (ret != OK) {
		return ret;
	}

	cfg->delay_ms_cb(100);

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
