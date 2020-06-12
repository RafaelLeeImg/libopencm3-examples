/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>

#define LD2_PORT GPIOA
#define LD2_PIN GPIO5

#define CONSOLE_TX_PORT GPIOA
#define CONSOLE_TX_PIN GPIO2
#define CONSOLE_TX_AF GPIO_AF4

#define CONSOLE_RX_PORT GPIOA
#define CONSOLE_RX_PIN GPIO3
#define CONSOLE_RX_AF GPIO_AF4

#define CONSOLE_USART USART2

static void clock_setup(void)
{
	/* Enable GPIOC clock for LED & USARTs. */
	rcc_periph_clock_enable(RCC_GPIOA);

	/* Enable clocks for USART2. */
	rcc_periph_clock_enable(RCC_USART2);
}

static void usart_setup(void)
{
	/* Enable the USART2 interrupt. */
	nvic_enable_irq(NVIC_USART2_IRQ);

	/* Setup USART2 parameters. */
	usart_set_baudrate(USART2, 115200);
	usart_set_databits(USART2, 8);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_stopbits(USART2, USART_CR2_STOPBITS_1);
	usart_set_mode(USART2, USART_MODE_TX_RX);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART2);

	/* Enable USART2 Receive interrupt. */
	usart_enable_rx_interrupt(USART2);
}

static void gpio_setup(void)
{
	/* Setup GPIO pin GPIO8/9 on GPIO port C for LEDs. */
	// gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);

	/* Setup GPIO pins for USART2 transmit. */
	gpio_mode_setup(LD2_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LD2_PIN);

	// /* Setup USART2 TX pin as alternate function. */
	// described in
	// DM00104451 - PM0223 - Programming manual Cortex®-M0+ programming manual
	// for STM32L0, STM32G0, STM32WL and STM32WB Series(Rev 13).pdf
	//
	// Table 16. Alternate function port A

	gpio_mode_setup(CONSOLE_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, CONSOLE_TX_PIN);
	gpio_mode_setup(CONSOLE_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, CONSOLE_RX_PIN);

	gpio_set_af(CONSOLE_TX_PORT, CONSOLE_TX_AF, CONSOLE_TX_PIN);
	gpio_set_af(CONSOLE_RX_PORT, CONSOLE_RX_AF, CONSOLE_RX_PIN);
}

int main(void)
{
	int i, j = 0, c = 0;

	clock_setup();
	gpio_setup();
	usart_setup();

	uint8_t data = 0;
	while (1);

	return 0;
}

void usart2_isr(void)
{
	static uint8_t data = 'A';

	/* Check if we were called because of RXNE. */
	if (((USART_CR1(USART2) & USART_CR1_RXNEIE) != 0) &&
		((USART_ISR(USART2) & USART_ISR_RXNE) != 0)) {

		/* Indicate that we got data. */
		gpio_toggle(LD2_PORT, LD2_PIN); /* LED on/off */

		/* Retrieve the data from the peripheral. */
		data = usart_recv(USART2);

		/* Enable transmit interrupt so it sends back the data. */
		usart_enable_tx_interrupt(USART2);
	}

	/* Check if we were called because of TXE. */
	if (((USART_CR1(USART2) & USART_CR1_TXEIE) != 0) &&
		((USART_ISR(USART2) & USART_ISR_TXE) != 0)) {

		/* Put data into the transmit register. */
		usart_send(USART2, data);

		/* Disable the TXE interrupt as we don't need it anymore. */
		usart_disable_tx_interrupt(USART2);
	}
}
