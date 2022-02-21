# Pimoroni Inky Display Driver

The Pimoroni Inky Display Driver C implementation written by Tyler J.
Anderson and is based on the Python implementation published by
Pimoroni Ltd. (see link below).

Pimoroni and Inky are trademarks of Pimoroni Ltd. The author(s) of
this project are not affiliated with or endorsed by Pimoroni Ltd.

## Introduction

Pimoroni Ltd. provides a range of small E-ink displays perfect for
maker projects; however, the drivers supplied by Pimoroni Ltd. are
limited to Raspberry Pi systems running Raspbian.

This project intends to provide a driver interface to Pimoroni Inky
displays that can be adapted to any system with SPI and a working
C standard library implementation.

This library will not only implement the features present in the
original Python library, but it will eventually extend the original
functionality with the following features:

- Update only changed regions
- Overwrite only mode
- Character framebuffers

Note that these features will be implemented in the future once the
driver has verified all the functionality of the original Python
library written by Pimoroni Ltd.

## License

This library is Copyright 2022 Tyler J. Anderson and is free and open
source software. It is released under the terms of the BSD 3-Clause
license. The Pimoroni Inky Display Driver also includes a copy of the
Munit Testing Framework, which is Copyright 2013-2016 Evan Nemerson
and is licensed for use under the MIT License.

See [LICENSE](LICENSE) for details.

## Usage

Include the meta-header in your code. The location may vary depending
on your project structure.

``` c
#include "inky.h"
```

This library requires the user to write a Hardware Abstraction Layer
(HAL) to interface the system SPI and GPIO devices with the Inky
driver.

The following are prototypes for the required user callback functions.
Return `INKY_OK` (or 0) on success. Return less than 0 on failure. See
[inky-api.h](include/inky-api.h) for a list of standard INKY error
codes.

``` c
inky_error_state inky_hal_gpio_initialize(void *intf_ptr)
{
	/* Your HAL code here */
	
	return INKY_OK;
}

inky_error_state inky_hal_gpio_setup_pin(inky_pin gpin,
					    inky_gpio_direction gdir,
					    inky_pin_state gstate,
					    inky_gpio_pull_up_down gcfg,
					    void *intf_ptr)
{
	/* Your HAL code here */
	
	return INKY_OK;
}

inky_error_state inky_hal_gpio_output_state(inky_pin gpin,
					       inky_pin_state gstate,
					       void *intf_ptr)
{
	/* Your HAL code here */
	
	return INKY_OK;
}

inky_error_state inky_hal_gpio_input_state(inky_pin gpin,
					      inky_pin_state* out,
					      void *intf_ptr)
{
	/* Your HAL code here */
	
	return INKY_OK;
}

inky_error_state inky_hal_gpio_poll_pin(inky_pin gpin,
					   uint16_t timeout,
					   void *intf_ptr)
{
	/* Your HAL code here */
	
	return INKY_OK;
}

inky_error_state inky_hal_spi_setup(void *intf_ptr)
{
	/* Your HAL code here */
	
	return INKY_OK;
}

inky_error_state inky_hal_delay(uint32_t delay_us, void *intf_ptr)
{
	/* Your HAL code here */
	
	return INKY_OK;
}

inky_error_state inky_hal_spi_write(const uint8_t* buf, uint32_t len,
				     void *intf_ptr)
{
	/* Your HAL code here */
	
	return INKY_OK;
}

inky_error_state inky_hal_spi_write16(const uint16_t* buf, uint32_t len,
					 void *intf_ptr)
{
	/* Your HAL code here */
	
	return INKY_OK;
}
```

Once all the callbacks are defined, add them to the inky_config
structure.

``` c
void your_set_callbacks(inky_config *dev)
{
	/* Set callbacks */
	dev->gpio_init_cb = inky_hal_gpio_initialize;
	dev->gpio_setup_pin_cb = inky_hal_gpio_setup_pin;
	dev->gpio_output_cb = inky_hal_gpio_output_state;
	dev->gpio_input_cb = inky_hal_gpio_input_state;
	dev->gpio_poll_cb = inky_hal_gpio_poll_pin;
	dev->spi_setup_cb = inky_hal_spi_setup;
	dev->spi_write_cb = inky_hal_spi_write;
	dev->spi_write16_cb = inky_hal_spi_write16;
	dev->delay_us_cb = inky_hal_delay;
}
```

See the `inky_config` struct in [inky-api.h](include/inky-api.h) for
additional options to set prior to initializing the library.

The `inky_setup()` function must be called prior to using the hardware,
and `inky_free()` function must be called to release framebuffer and
other resources when your are finished with the library.

``` c
int main()
{
	int rst;
	inky_config dev;
	
	/* Your dev and interface setup functions here */
	
	rst = inky_setup(&dev); /* Must call before library functions */
	your_error_handler(rst);
	
	/* Your application and Inky Library functions here */
	
	rst = inky_free(&dev); /* Must call when done with inky library */
	your_error_handler(rst);
	
	return 0;
}
```

## Links


The Inky What and Inky Phat displays can be purchased at:

* [Pimoroni](https://shop.pimoroni.com/products/inky-what?variant=13590497624147)

This driver is based on the Python driver for Raspbian available at:

* [Pimoroni GitHub Repository](https://github.com/pimoroni/inky)
