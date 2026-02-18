#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

#include <stdarg.h>

LOG_MODULE_REGISTER(cdc_acm_echo, LOG_LEVEL_INF);

#define RING_BUF_SIZE 1024
uint8_t ring_buffer_rx[RING_BUF_SIZE];
struct ring_buf ringbuf_rx;

#define RING_BUF_SIZE 1024
uint8_t ring_buffer_tx[RING_BUF_SIZE];
struct ring_buf ringbuf_tx;

static void interrupt_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);
	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
			int recv_len, rb_len;
			uint8_t buffer[64];
			size_t len = MIN(ring_buf_space_get(&ringbuf_rx), sizeof(buffer));
			printk("Reciving up to %u bytes", len);
			recv_len = uart_fifo_read(dev, buffer, len);
			if (recv_len < 0) {
				recv_len = 0;
			};

			rb_len = ring_buf_put(&ringbuf_rx, buffer, recv_len);
			if (rb_len < recv_len) {
				printk("Drop %u bytes", recv_len - rb_len);
			}
		}

		if (uart_irq_tx_ready(dev)) {
			uint8_t buffer[64];
			int rb_len, send_len;

			rb_len = ring_buf_get(&ringbuf_tx, buffer, sizeof(buffer));
			if (!rb_len) {
				printk("Ring buffer empty, disable TX IRQ");
				uart_irq_tx_disable(dev);
				continue;
			}

			send_len = uart_fifo_fill(dev, buffer, rb_len);
			if (send_len < rb_len) {
				printk("Drop %d bytes", rb_len - send_len);
			}
		}
	}
}

const struct device *dev;

int initUsb()
{
	int ret;

	dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
	if (!device_is_ready(dev)) {
		printk("CDC ACM device not ready");
		return -1;
	}

	ret = usb_enable(NULL);

	if (ret != 0) {
		printk("Failed to enable USB");
		return -1;
	}

	ring_buf_init(&ringbuf_tx, sizeof(ring_buffer_tx), ring_buffer_tx);
	ring_buf_init(&ringbuf_rx, sizeof(ring_buffer_rx), ring_buffer_rx);

	uart_irq_callback_set(dev, interrupt_handler);

	/* Enable rx interrupts */
	uart_irq_rx_enable(dev);

	return 0;
}

void waitForUsb()
{
	while (true) {
		uint32_t dtr = 0U;
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		if (dtr) {
			break;
		} else {
			/* Give CPU resources to low priority threads. */
			k_sleep(K_MSEC(100));
		}
	}

	/* Wait 100ms for the host to do all settings */
	k_msleep(100);
}

/// @return Amount of bytes written to data.
int usbRead(char *data, uint32_t size)
{
	return ring_buf_get(&ringbuf_rx, (uint8_t *)data, size);
}

/// @return Amount of bytes written to uart.
int usbWrite(const uint8_t *data, uint32_t size)
{
	int res = ring_buf_put(&ringbuf_tx, data, size);
	uart_irq_tx_enable(dev);
	return res;
}

int usbRxBufferLen()
{
	return ring_buf_size_get(&ringbuf_rx);
}

int usbTxBufferLen()
{
	return ring_buf_size_get(&ringbuf_tx);
}

char buffer[256];

int printu(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	int count = vsnprintf(&buffer, 256, format, args);
	int res = usbWrite(&buffer, count);

	va_end(args);

	return res;
}

int printuln(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	int count = vsnprintf(&buffer, 256, format, args);
	int res = usbWrite(&buffer, count);
	res += usbWrite("\r\n", 2);

	va_end(args);

	return res;
}
