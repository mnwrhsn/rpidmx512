/**
 * @file mcp23s17.c
 *
 */
/* Copyright (C) 2015, 2016 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifdef __AVR_ARCH__
#include "util/delay.h"
#include "avr_spi.h"
#else
#include "bcm2835.h"
#ifdef BARE_METAL
#include "bcm2835_spi.h"
#endif
#endif
#include "device_info.h"
#include "mcp23s17.h"

#ifdef __AVR_ARCH__
#define FUNC_PREFIX(x) avr_##x
#else
#define FUNC_PREFIX(x) bcm2835_##x
#endif

/**
 * @ingroup SPI-DIO
 *
 * @param device_info
 */
inline void static mcp23s17_setup(const device_info_t *device_info)
{
#ifdef __AVR_ARCH__
#else
	bcm2835_spi_setClockDivider(device_info->internal_clk_div);
	bcm2835_spi_chipSelect(device_info->chip_select);
	bcm2835_spi_setChipSelectPolarity(device_info->chip_select, LOW);
#endif
}

/**
 * @ingroup SPI-DIO
 *
 * @param device_info
 * @return
 */
uint8_t mcp23s17_start(device_info_t *device_info) {
#if !defined(BARE_METAL) && !defined(__AVR_ARCH__)
	if (bcm2835_init() != 1)
		return MCP23S17_ERROR;
#endif
	FUNC_PREFIX(spi_begin());

	if (device_info->slave_address == (uint8_t) 0) {
		device_info->slave_address = MCP23S17_DEFAULT_SLAVE_ADDRESS;
	} else {
		device_info->slave_address = device_info->slave_address & 0x03;
	}

	if (device_info->speed_hz == (uint32_t) 0) {
		device_info->speed_hz = (uint32_t) MCP23S17_SPI_SPEED_DEFAULT_HZ;
	} else if (device_info->speed_hz > (uint32_t) MCP23S17_SPI_SPEED_MAX_HZ) {
		device_info->speed_hz = (uint32_t) MCP23S17_SPI_SPEED_MAX_HZ;
	}

	device_info->internal_clk_div = (uint16_t)((uint32_t) BCM2835_CORE_CLK_HZ / device_info->speed_hz);

	mcp23s17_reg_write_byte(device_info, MCP23S17_IOCON, MCP23S17_IOCON_HAEN);

	return MCP23S17_OK;
}

/**
 * @ingroup SPI-DIO
 *
 */
void mcp23s17_end(void) {
	FUNC_PREFIX(spi_end());
}

/**
 * @ingroup SPI-DIO
 *
 * @param device_info
 * @param reg
 * @return
 */
uint16_t mcp23s17_reg_read(const device_info_t *device_info, const uint8_t reg) {
	char spiData[4];
	spiData[0] = (char) MCP23S17_CMD_READ | (char) ((device_info->slave_address) << 1);
	spiData[1] = (char) reg;

	mcp23s17_setup(device_info);
	FUNC_PREFIX(spi_transfern(spiData, 4));
	return  ((uint16_t)spiData[2] | ((uint16_t)spiData[3] << 8));
}

/**
 * @ingroup SPI-DIO
 *
 * @param device_info
 * @param reg
 * @param value
 */
void mcp23s17_reg_write(const device_info_t *device_info, const uint8_t reg, const uint16_t value) {
	char spiData[4];
	spiData[0] = (char) MCP23S17_CMD_WRITE | (char) ((device_info->slave_address) << 1);
	spiData[1] = (char) reg;
	spiData[2] = (char) value;
	spiData[3] = (char) (value >> 8);

	mcp23s17_setup(device_info);
	FUNC_PREFIX(spi_writenb(spiData, 4));
}

/**
 * @ingroup SPI-DIO
 *
 * @param device_info
 * @param reg
 * @param value
 */
void mcp23s17_reg_write_byte(const device_info_t *device_info, const uint8_t reg, const uint8_t value) {
	char spiData[3];
	spiData[0] = (char) MCP23S17_CMD_WRITE | (char) ((device_info->slave_address) << 1);
	spiData[1] = (char) reg;
	spiData[2] = (char) value;

	mcp23s17_setup(device_info);
	FUNC_PREFIX(spi_writenb(spiData, 3));
}


/**
 * @ingroup SPI-DIO
 * Sets the Function Select register for the given pin, which configures
 * the pin as Input, Output
 * @param device_info
 * @param pin GP number, or one of MCP23S17_PIN_* from \ref mcp23s08Pin.
 * @param mode Mode to set the pin to, one of MCP23S17_FSEL_* from \ref mcp23s08FunctionSelect
 */
void mcp23s17_gpio_fsel(const device_info_t *device_info, const uint16_t pin, const uint8_t mode) {
	uint8_t data = mcp23s17_reg_read(device_info, MCP23S17_IODIRA);

	if (mode == MCP23S17_FSEL_OUTP) {
		data &= (~pin);
	} else {
		data |= pin;
	}

	mcp23s17_reg_write(device_info, MCP23S17_IODIRA, data);
}

/**
 * @ingroup SPI-DIO
 * Sets the specified pin output to low.
 * @param device_info
 * @param pin GP number, or one of MCP23S17_PIN_* from \ref mcp23s17Pin.
 */
void mcp23s17_gpio_set(const device_info_t *device_info, const uint16_t pin) {
	uint8_t data = mcp23s17_reg_read(device_info, MCP23S17_OLATA);
	data |= pin;
	mcp23s17_reg_write(device_info, MCP23S17_GPIOA, data);
}

/**
 * @ingroup SPI-DIO
 * Sets the specified pin output to low.
 * @param device_info
 * @param pin GP number, or one of MCP23S17_PIN_* from \ref mcp23s17Pin.
 */
void mcp23s17_gpio_clr(const device_info_t *device_info, const uint16_t pin) {
	uint8_t data = mcp23s17_reg_read(device_info, MCP23S17_OLATA);
	data &= (~pin);
	mcp23s17_reg_write(device_info, MCP23S17_GPIOA, data);
}
