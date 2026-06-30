#include "bflb_uart.h"
#include "board.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "usbh_core.h"
#include "usbh_hid.h"

#include "FreeRTOS.h"
#include "task.h"

static volatile int hid_in_nbytes;
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t hid_buffer[8];

static uint8_t last_report[8];

static void uart_write(uint8_t ch)
{
    struct bflb_device_s *uartx = bflb_device_get_by_name(DEFAULT_TEST_UART);
    if (!uartx) {
        return;
    }

    bflb_uart_putchar(uartx, ch);
}

static char hid_key_to_ascii(uint8_t key, uint8_t modifiers)
{
    bool shift = (modifiers & 0x22) != 0;
    bool ctrl = (modifiers & 0x11) != 0;

    switch (key) {
        // --- Alphabet keys ---
        case 0x04: return shift ? 'A' : ctrl ? 0x01 : 'a';
        case 0x05: return shift ? 'B' : ctrl ? 0x02 : 'b';
        case 0x06: return shift ? 'C' : ctrl ? 0x03 : 'c';
        case 0x07: return shift ? 'D' : ctrl ? 0x04 : 'd';
        case 0x08: return shift ? 'E' : ctrl ? 0x05 : 'e';
        case 0x09: return shift ? 'F' : ctrl ? 0x06 : 'f';
        case 0x0A: return shift ? 'G' : ctrl ? 0x07 : 'g';
        case 0x0B: return shift ? 'H' : ctrl ? 0x08 : 'h';
        case 0x0C: return shift ? 'I' : ctrl ? 0x09 : 'i';
        case 0x0D: return shift ? 'J' : ctrl ? 0x0A : 'j';
        case 0x0E: return shift ? 'K' : ctrl ? 0x0B : 'k';
        case 0x0F: return shift ? 'L' : ctrl ? 0x0C : 'l';
        case 0x10: return shift ? 'M' : ctrl ? 0x0D : 'm';
        case 0x11: return shift ? 'N' : ctrl ? 0x0E : 'n';
        case 0x12: return shift ? 'O' : ctrl ? 0x0F : 'o';
        case 0x13: return shift ? 'P' : ctrl ? 0x10 : 'p';
        case 0x14: return shift ? 'Q' : ctrl ? 0x11 : 'q';
        case 0x15: return shift ? 'R' : ctrl ? 0x12 : 'r';
        case 0x16: return shift ? 'S' : ctrl ? 0x13 : 's';
        case 0x17: return shift ? 'T' : ctrl ? 0x14 : 't';
        case 0x18: return shift ? 'U' : ctrl ? 0x15 : 'u';
        case 0x19: return shift ? 'V' : ctrl ? 0x16 : 'v';
        case 0x1A: return shift ? 'W' : ctrl ? 0x17 : 'w';
        case 0x1B: return shift ? 'X' : ctrl ? 0x18 : 'x';
        case 0x1C: return shift ? 'Y' : ctrl ? 0x19 : 'y';
        case 0x1D: return shift ? 'Z' : ctrl ? 0x1A : 'z';

        // --- Number Row ---
        case 0x1E: return shift ? '!' : '1';
        case 0x1F: return shift ? '@' : '2';
        case 0x20: return shift ? '#' : '3';
        case 0x21: return shift ? '$' : '4';
        case 0x22: return shift ? '%' : '5';
        case 0x23: return shift ? '^' : '6';
        case 0x24: return shift ? '&' : '7';
        case 0x25: return shift ? '*' : '8';
        case 0x26: return shift ? '(' : '9';
        case 0x27: return shift ? ')' : '0';

        // --- Structural / Control keys ---
        case 0x28: return '\n';     // Return / Enter
        case 0x29: return 0x1B;     // Escape
        case 0x2A: return '\b';     // Backspace
        case 0x2B: return '\t';     // Tab
        case 0x2C: return ' ';      // Spacebar

        // --- Remainder of main keyboard symbols ---
        case 0x2D: return shift ? '_' : '-';
        case 0x2E: return shift ? '+' : '=';
        case 0x2F: return shift ? '{' : '[';
        case 0x30: return shift ? '}' : ']';
        case 0x31: return shift ? '|' : '\\';
        case 0x33: return shift ? ':' : ';';
        case 0x34: return shift ? '"' : '\'';
        case 0x35: return shift ? '~' : '`';
        case 0x36: return shift ? '<' : ',';
        case 0x37: return shift ? '>' : '.';
        case 0x38: return shift ? '?' : '/';

        // --- Keypad math symbols ---
        case 0x54: return '/';      // Keypad /
        case 0x55: return '*';      // Keypad *
        case 0x56: return '-';      // Keypad -
        case 0x57: return '+';      // Keypad +
        case 0x58: return '\n';     // Keypad Enter

        // --- Keypad numbers (fallback if NumLock isn't handled upstream) ---
        case 0x59: return '1';
        case 0x5A: return '2';
        case 0x5B: return '3';
        case 0x5C: return '4';
        case 0x5D: return '5';
        case 0x5E: return '6';
        case 0x5F: return '7';
        case 0x60: return '8';
        case 0x61: return '9';
        case 0x62: return '0';
        case 0x63: return '.';

        default: return 0;          // Unmapped key (F-keys, arrows, etc.)
    }
}

static void handle_keyboard_report(const uint8_t *report, int nbytes)
{
    char line[96];
    int pos = 0;
    uint8_t modifiers = report[0];
    int report_len = nbytes < sizeof(last_report) ? nbytes : sizeof(last_report);

    pos += snprintf(line + pos, sizeof(line) - pos, "HID mod=0x%02x keys=", modifiers);

    bool printed_any = false;
    for (int i = 2; i < report_len; i++) {
        bool is_new_key = true;

        if (report[i] == 0) {
            continue;
        }

        for (int j = 2; j < sizeof(last_report); j++) {
            if (report[i] == last_report[j]) {
                is_new_key = false;
                break;
            }
        }

        // convert scan code and modifiers to ascii
        char ch = hid_key_to_ascii(report[i], modifiers);
        if (ch != 0) {
            line[pos++] = ch;
            printed_any = true;

            if (is_new_key) {
                uart_write(ch);
            }
        } else {
            pos += snprintf(line + pos, sizeof(line) - pos, "[%02x]", report[i]);
            printed_any = true;
        }
    }

    if (!printed_any) {
        pos += snprintf(line + pos, sizeof(line) - pos, "<none>");
    }

    pos += snprintf(line + pos, sizeof(line) - pos, "\r\n");

    USB_LOG_RAW("%s", line);

    // save report in last_report for comparison later
    memset(last_report, 0, sizeof(last_report));
    memcpy(last_report, report, report_len);
}

void usbh_hid_in_callback(void *arg, int nbytes)
{
    hid_in_nbytes = nbytes;
}

static void usbh_hid_thread(CONFIG_USB_OSAL_THREAD_SET_ARGV)
{
    int ret;
    uint32_t hid_interval_ms;
    struct usbh_hid *hid_class = (struct usbh_hid *)CONFIG_USB_OSAL_THREAD_GET_ARGV;

    static bool run_flag = false;
    if (run_flag == true) {
        USB_LOG_ERR("USBH HID bridge already running\r\n");
        goto exit_del;
    }
    run_flag = true;

    if (hid_class->hport->speed == USB_SPEED_HIGH) {
        hid_interval_ms = (0x01 << (hid_class->intin->bInterval - 1)) * 125 / 1000;
    } else {
        hid_interval_ms = hid_class->intin->bInterval;
    }

    USB_LOG_RAW("USB HID bridge started\r\n");

    // initialise and clear last_report for holding previous hid report
    memset(last_report, 0, sizeof(last_report));

    while (1) {
        usbh_int_urb_fill(&hid_class->intin_urb, hid_class->hport, hid_class->intin, hid_buffer, hid_class->intin->wMaxPacketSize, 1000, usbh_hid_in_callback, hid_class);
        ret = usbh_submit_urb(&hid_class->intin_urb);
        if (ret < 0) {
            if (ret == -USB_ERR_TIMEOUT) {
                continue;
            }
            USB_LOG_ERR("hid submit urb error, ret:%d\r\n", ret);
            goto exit_del;
        }

        if (hid_in_nbytes < 0) {
            USB_LOG_ERR("hid in error, ret:%d\r\n", hid_in_nbytes);
        } else if (hid_in_nbytes > 0) {
            handle_keyboard_report(hid_buffer, hid_in_nbytes);
        }

        usb_osal_msleep(hid_interval_ms);
    }

exit_del:
    run_flag = false;
    USB_LOG_RAW("USB HID bridge stopped\r\n");
    usb_osal_thread_delete(NULL);
}

void usbh_hid_run(struct usbh_hid *hid_class)
{
    usb_osal_thread_create("usbh_hid", 2048, CONFIG_USBHOST_PSC_PRIO - 1, usbh_hid_thread, hid_class);
}

void usbh_hid_stop(struct usbh_hid *hid_class)
{
}
