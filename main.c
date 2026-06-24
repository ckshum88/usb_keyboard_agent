#include "bflb_mtimer.h"
#include "bflb_uart.h"
#include "bflb_gpio.h"
#include "board.h"
#include "bflb_name.h"

#include <stdbool.h>

#include "usbh_core.h"

#include "FreeRTOS.h"
#include "task.h"

#define DBG_TAG "USB_KEYBOARD_AGENT"
#include "log.h"

static TaskHandle_t bridge_task_handle;
struct bflb_device_s *gpio, *uartx;

void usb_hid_bridge_task(void *arg)
{
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

int main(void)
{
    board_init();

#if defined(BOARD_USB_VIA_GPIO)
    board_usb_gpio_init();
#endif

    struct bflb_device_s *gpio = bflb_device_get_by_name("gpio");
    if (gpio) {
        bflb_gpio_uart_init(gpio, GPIO_PIN_16, GPIO_UART_FUNC_UART1_TX);
        bflb_gpio_uart_init(gpio, GPIO_PIN_17, GPIO_UART_FUNC_UART1_RX);
    }

    struct bflb_device_s *uartx = bflb_device_get_by_name(DEFAULT_TEST_UART);
    if (uartx) {
        struct bflb_uart_config_s cfg;
        cfg.baudrate = 115200;
        cfg.data_bits = UART_DATA_BITS_8;
        cfg.stop_bits = UART_STOP_BITS_1;
        cfg.parity = UART_PARITY_NONE;
        cfg.flow_ctrl = 0;
        cfg.tx_fifo_threshold = 7;
        cfg.rx_fifo_threshold = 7;
        bflb_uart_init(uartx, &cfg);
        LOG_I("UART1 initialized on GPIO 16/17 at 115200 baud\r\n");
    } else {
        LOG_E("UART1 device not found\r\n");
    }

    struct bflb_device_s *usb_dev = bflb_device_get_by_name(BFLB_NAME_USB_V2);
    if (usb_dev) {
        usbh_initialize(0, usb_dev->reg_base);
        LOG_I("USB host stack initialized\r\n");
    } else {
        LOG_E("USB device not found\r\n");
    }

    xTaskCreate(usb_hid_bridge_task, (char *)"usb_hid_bridge", 2048, NULL, configMAX_PRIORITIES - 2, &bridge_task_handle);

    LOG_I("USB keyboard agent started\r\n");
    vTaskStartScheduler();
    return 0;
}
