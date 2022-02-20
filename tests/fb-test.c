/**
 * @file fb-test.c
 *
 * Unit testing for the driver for the Pimoroni Inky
 */

#include "inky.h"

#include <munit/munit.h>

#include <stdint.h>

#define INTF(ptr)				\
	struct test_intf *intf = (struct test_intf*) ptr;

struct test_intf {
	uint8_t *last_bytes_out;
	uint32_t n_bytes_out;
	uint8_t *last_bytes_in;
	uint32_t n_bytes_in;
	inky_color_config color;
	inky_config dev;
	void *usrptr;
	uint8_t *buf;
};

inky_error_state inky_tests_gpio_initialize(void *intf_ptr)
{
	return INKY_OK;
}

inky_error_state inky_tests_gpio_setup_pin(inky_pin gpin,
					    inky_gpio_direction gdir,
					    inky_pin_state gstate,
					    inky_gpio_pull_up_down gcfg,
					    void *intf_ptr)
{
	return INKY_OK;
}

inky_error_state inky_tests_gpio_output_state(inky_pin gpin,
					       inky_pin_state gstate,
					       void *intf_ptr)
{
	return INKY_OK;
}

inky_error_state inky_tests_gpio_input_state(inky_pin gpin,
					      inky_pin_state* out,
					      void *intf_ptr)
{
	return INKY_OK;
}

inky_error_state inky_tests_gpio_poll_pin(inky_pin gpin,
					   uint16_t timeout,
					   void *intf_ptr)
{	return INKY_OK;
}

inky_error_state inky_tests_spi_setup(void *intf_ptr)
{
	/* Initialize test structures for accessing bytes sent
	 * by SPI */
	INTF(intf_ptr);

	intf->last_bytes_in = NULL;
	intf->last_bytes_out = NULL;
	intf->n_bytes_in = 0;
	intf->n_bytes_out = 0;

	return INKY_OK;
}

inky_error_state inky_tests_delay(uint32_t delay_us, void *intf_ptr)
{
	return INKY_OK;
}

inky_error_state inky_tests_spi_write(const uint8_t* buf, uint32_t len,
				     void *intf_ptr)
{
	INTF(intf_ptr);

	intf->last_bytes_out = realloc(intf->last_bytes_out, len);

	if (!intf->last_bytes_out) {
		return INKY_E_OUT_OF_MEMORY;
	}

	for (uint32_t i = 0; i < len; i++) {
		intf->last_bytes_out[i] = buf[i];
	}

	intf->n_bytes_out = len;

	return INKY_OK;
}

inky_error_state inky_tests_spi_write16(const uint16_t* buf, uint32_t len,
					 void *intf_ptr)
{
	uint32_t bytes_len;

	INTF(intf_ptr);

	bytes_len = len * 2;

	intf->last_bytes_out = realloc(intf->last_bytes_out, bytes_len);

	if (!intf->last_bytes_out) {
		return INKY_E_OUT_OF_MEMORY;
	}

	for (uint32_t i = 0; i < bytes_len; i = i + 2) {
		uint32_t wi = i / 2;

		/* MSB first */
		intf->last_bytes_out[i] = (uint8_t) (buf[wi] >> 8);
		intf->last_bytes_out[i + 1] = (uint8_t) (buf[wi] & 0x00ff);
	}

	intf->n_bytes_out = bytes_len;

	return INKY_OK;
}

void initialize_test_device(struct test_intf *intf)
{
	inky_config *dev = &intf->dev;

	/* Only INKY_WHAT is currently supported */
	dev->pdt = INKY_WHAT;

	/* Set up colors */
	intf->color.white = 1;
	intf->color.red = 1;
	intf->color.black = 1;
	intf->color.yellow = 0;

	/* Set interface ptr */
	dev->intf_ptr = (void*) intf;
	dev->color = &intf->color;

	/* Set callbacks */
	dev->gpio_init_cb = inky_tests_gpio_initialize;
	dev->gpio_setup_pin_cb = inky_tests_gpio_setup_pin;
	dev->gpio_output_cb = inky_tests_gpio_output_state;
	dev->gpio_input_cb = inky_tests_gpio_input_state;
	dev->gpio_poll_cb = inky_tests_gpio_poll_pin;
	dev->spi_setup_cb = inky_tests_spi_setup;
	dev->spi_write_cb = inky_tests_spi_write;
	dev->spi_write16_cb = inky_tests_spi_write16;
	dev->delay_us_cb = inky_tests_delay;

	/* Zero-initialize other options */
	dev->fb = NULL;
	dev->active_fb = NULL;
	dev->exclude_flags = 0;
	dev->usrptr1 = NULL;
	dev->usrptr2 = NULL;
	intf->usrptr = NULL;
	intf->buf = NULL;
}

void deinitialize_test_device(struct test_intf *intf)
{
	free(intf->last_bytes_in);
	free(intf->last_bytes_out);
}

uint32_t create_random_image(struct test_intf * intf)
{
	uint8_t cdepth;
	uint64_t len_pixels;
	uint32_t len_bytes;

	inky_config *dev = &intf->dev;

	/* Make sure our pointers are good to go */
	munit_assert_ptr_not_null(dev);
	munit_assert_ptr_not_null(dev->color);
	munit_assert_ptr_not_null(dev->fb);

	cdepth = dev->color->red || dev->color->yellow ? 2 : 1;

	/* Allocate fake image buffer */
	len_pixels = dev->fb->width * dev->fb->height;
	len_bytes = len_pixels * 2 / 8;
	intf->buf = malloc(len_bytes);

	if (!intf->buf) {
		return -1;
	}

	for (uint32_t i = 0; i < len_bytes; i++) {
		intf->buf[i] = 0;

		for (uint8_t j = 0; j < 8; j = j + 2) {
			uint8_t pixel;

			/* Generate random color */
			pixel = (uint8_t) munit_rand_int_range(0, cdepth);
			pixel = (pixel & 0x03) << j;

			/* Assign bits to image */
			intf->buf[i] = intf->buf[i] | pixel;
		}
	}

	return len_bytes;
}

void destroy_random_image(uint8_t *img_buf)
{
	free(img_buf);
}

/**
 * @defgroup inky-init-test Test inky initialization
 * @{
 */

static void *inky_init_setup(const MunitParameter params[],
			     void *user_data)
{
	INTF(user_data);

	initialize_test_device(intf);

	return user_data;
}

static void inky_init_tear_down(void *fixture)
{
	INTF(fixture);

	inky_free(fixture);

	deinitialize_test_device(intf);
}

MunitResult inky_init_test(const MunitParameter params[],
			   void *user_data)
{
	INTF(user_data);
	inky_config *dev = (inky_config*) &intf->dev;

	munit_assert_int8(inky_setup(dev), ==, INKY_OK);

	return MUNIT_OK;
}

/**
 * @}
 */

/**
 * @defgroup inky-clear-test Test inky initialization
 * @{
 */

static void *inky_clear_setup(const MunitParameter params[],
			     void *user_data)
{
	INTF(user_data);

	initialize_test_device(intf);

	return user_data;
}

static void inky_clear_tear_down(void *fixture)
{
	INTF(fixture);

	inky_free(fixture);

	deinitialize_test_device(intf);
}

MunitResult inky_clear_test(const MunitParameter params[],
			   void *user_data)
{
	INTF(user_data);
	inky_config *dev = (inky_config*) &intf->dev;

	munit_assert_int8(inky_setup(dev), ==, INKY_OK);

	munit_assert_int8(inky_clear(dev), ==, INKY_OK);

	return MUNIT_OK;
}

/**
 * @}
 */

/**
 * @defgroup fb-usrptr-test Test inky initialization
 * @{
 */

static void *fb_usrptr_setup(const MunitParameter params[],
			     void *user_data)
{
	int arr_len = 8;

	INTF(user_data);

	initialize_test_device(intf);

	intf->buf = munit_malloc(arr_len);

	for (int i = 0; i < arr_len; i++) {
		intf->buf[i] = 0x42;
	}

	return user_data;
}

static void fb_usrptr_tear_down(void *fixture)
{
	INTF(fixture);

	inky_free(fixture);

	free(intf->buf);

	deinitialize_test_device(intf);
}

MunitResult fb_usrptr_test(const MunitParameter params[],
			   void *user_data)
{
	const unsigned int max_n_usrptr = 2;

	INTF(user_data);
	inky_config *dev = (inky_config*) &intf->dev;

	munit_assert_int8(inky_setup(dev), ==, INKY_OK);

	for (unsigned int i = 0; i < max_n_usrptr + 1; i++) {
		uint8_t ret;

		if (i < max_n_usrptr) {
			ret = INKY_OK;
		} else {
			ret = INKY_E_NOT_AVAILABLE;
		}

		munit_assert_int8(inky_fb_usrptr_attach(dev, i + 1,
							(void*) intf->buf),
				  ==, ret);

		if (i == 0) {
			munit_assert_not_null(dev->fb->usrptr1);
		} else if (i == 1) {
			munit_assert_not_null(dev->fb->usrptr2);
		}
	}

	return MUNIT_OK;
}

/**
 * @}
 */

/**
 * @defgroup random-fb-test Framebuffer test group
 * @{
 */

static void *random_fb_setup(const MunitParameter params[],
			     void *user_data)
{
	INTF(user_data);

	initialize_test_device(intf);

	return user_data;
}

static void random_fb_tear_down(void *fixture)
{
	INTF(fixture);

	destroy_random_image((uint8_t*) intf->usrptr);

	inky_free(fixture);

	deinitialize_test_device(intf);
}

MunitResult random_fb_test(const MunitParameter params[],
			   void *user_data)
{
	uint32_t buf_len;

	INTF(user_data);
	inky_config *dev = &intf->dev;

	munit_assert_int8(inky_setup(&intf->dev), ==, INKY_OK);

	buf_len = create_random_image(intf);
	munit_assert_not_null(intf->buf);
	munit_assert_uint32(buf_len, ==, (uint32_t) dev->fb->bytes);

	for (uint32_t i = 0; i < buf_len; i++) {
		for (uint8_t j = 0; j < 8; j = j + 2) {
			inky_color c;
			uint8_t pixel = (intf->buf[i] >> j) & 0x03;
			uint64_t addr = (i * 8 + j) / 2;

			uint16_t y = addr / dev->fb->width;
			uint16_t x = addr % dev->fb->width;

			munit_logf(MUNIT_LOG_DEBUG,
				   "Writing color %d at addr %lu "
				   "to (%u,%u)",
				   pixel, addr, x, y);

			if (pixel == 1) c = INKY_COLOR_BLACK;
			else if (pixel == 0) c = INKY_COLOR_WHITE;
			else c = INKY_COLOR_RED;

			munit_assert_int8(inky_fb_set_pixel(dev, x, y, c),
					  ==, INKY_OK);
		}
	}

	/* Check that the fb in the device is the same as the random
	 * image produced */
	munit_logf(MUNIT_LOG_DEBUG, "First four IMG: %#x %#x %#x %#x,"
		   " FB: %#x %#x %#x %#x",
		   intf->buf[0], intf->buf[1], intf->buf[2], intf->buf[3],
		   dev->fb->buffer[0], dev->fb->buffer[1],
		   dev->fb->buffer[2], dev->fb->buffer[3]);
	munit_assert_memory_equal(dev->fb->bytes, intf->buf, dev->fb->buffer);

	munit_assert_int8(inky_update(dev), ==, INKY_OK);

	return MUNIT_OK;
}

/**
 * @}
 */

MunitTest fb_tests[] = {
	{
		"/inky-init-test",
		inky_init_test,
		inky_init_setup,
		inky_init_tear_down,
		MUNIT_TEST_OPTION_NONE,
		NULL
	},

	{
		"/inky-clear-test",
		inky_clear_test,
		inky_clear_setup,
		inky_clear_tear_down,
		MUNIT_TEST_OPTION_NONE,
		NULL
	},

	{
		"/fb-usrptr-test",
		fb_usrptr_test,
		fb_usrptr_setup,
		fb_usrptr_tear_down,
		MUNIT_TEST_OPTION_NONE,
		NULL
	},

	{
		"/random-fb-test",
		random_fb_test,
		random_fb_setup,
		random_fb_tear_down,
		MUNIT_TEST_OPTION_NONE,
		NULL
	},

	{
		NULL,
		NULL,
		NULL,
		NULL,
		MUNIT_TEST_OPTION_NONE,
		NULL
	}
};

static const MunitSuite inky_suite = {
	"/inky-test-suite",
	fb_tests,
	NULL,
	5,
	MUNIT_SUITE_OPTION_NONE
};

int main(int argc, char *const argv[])
{
	struct test_intf intf;

	return munit_suite_main(&inky_suite, (void*) &intf, argc, argv);
}
