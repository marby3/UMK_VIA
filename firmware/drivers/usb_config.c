/* UMK_VIA - rv003usb callbacks.
 *
 * Everything here runs inside the USB interrupt. rv003usb ACKs the host as
 * soon as these functions return, so they must stay short: copy bytes, set a
 * flag, get out. All interpretation happens in the main loop.
 */

#include "ch32fun.h"
#include "rv003usb.h"
#include "usb_config.h"
#include "via.h"

volatile uint8_t usb_configured_flag = 0;

/* usb_send_data reads this as words, so keep it word aligned. */
uint8_t hid_keyboard_report[8] __attribute__((aligned(4))) = { 0 };

/* The host only enumerates the keyboard after it has seen us leave the bus.
 *
 * A cold plug gives that for free, but the USB bootloader hands over to user
 * code through a soft reset while the host may still be holding the
 * bootloader's own device (VID 0x1209 / PID 0xB003). Without a disconnect
 * edge the board just sits there looking unresponsive.
 *
 * With CUSTOM_USB_PIN_DPU configured, rv003usb's usb_setup() raises the D-
 * pull-up itself and that edge is the reconnect. When the 1.5k pull-up is
 * tied straight to 3V3 there is nothing to toggle, so hold D- low here
 * instead. USB's TDDIS is 2.5us; 10ms clears any host side debounce.
 */
void usb_force_reenumerate(void)
{
	funGpioInitAll();
	funDigitalWrite(UMK_USB_PIN_DM, FUN_LOW);
	funPinMode(UMK_USB_PIN_DM, GPIO_CFGLR_OUT_10Mhz_PP);
	Delay_Ms(10);
	funPinMode(UMK_USB_PIN_DM, GPIO_CFGLR_IN_FLOAT);
	Delay_Ms(1);
}

void usb_handle_user_in_request(struct usb_endpoint *e, uint8_t *scratchpad,
                                int endp, uint32_t sendtok,
                                struct rv003usb_internal *ist)
{
	(void)scratchpad;
	(void)ist;

	if (endp == 1) {
		usb_send_data(hid_keyboard_report, 8, 0, sendtok);
	} else if (endp == 2) {
		via_handle_in(e, sendtok);
	} else {
		usb_send_empty(sendtok);
	}
}

void usb_handle_user_data(struct usb_endpoint *e, int current_endpoint,
                          uint8_t *data, int len,
                          struct rv003usb_internal *ist)
{
	(void)e;
	(void)ist;

	if (current_endpoint == 2) {
		via_receive_packet(data, len);
	}
}

void usb_handle_other_control_message(struct usb_endpoint *e,
                                      struct usb_urb *s,
                                      struct rv003usb_internal *ist)
{
	(void)e;
	(void)ist;

	/* bmRequestType 0x00 | bRequest 0x09 = SET_CONFIGURATION. Reaching this
	 * means the host finished enumerating us. */
	if (s->wRequestTypeLSBRequestMSB == 0x0900) {
		usb_configured_flag = 1;
	}
}
