/* UMK_VIA - USB descriptors and rv003usb configuration.
 *
 * Three endpoints (spec 4.2):
 *   EP0        Control                  8 bytes
 *   EP1 IN     Boot Keyboard HID        8 bytes, bInterval 10ms
 *   EP2 IN/OUT VIA Raw HID   32 logical / 8 physical, bInterval 10ms
 *
 * Low-Speed USB caps every packet at 8 bytes and bInterval at 10ms; the 32
 * byte VIA report is therefore carried as 4 transactions and reassembled in
 * via.c.
 *
 * The Usage Page 0xFF60 / Usage 0x61 pair on EP2 is NOT negotiable: Remap's
 * WebHid.ts only recognises collections matching exactly that.
 */
#ifndef _USB_CONFIG_H
#define _USB_CONFIG_H

#include "config.h"

#define ENDPOINTS 3

/* USB D+/D- wiring. Overridable per keyboard, defaults match rv003usb's. */
#ifndef CUSTOM_USB_PORT
#define CUSTOM_USB_PORT D
#endif
#ifndef CUSTOM_USB_PIN_DP
#define CUSTOM_USB_PIN_DP 3
#endif
#ifndef CUSTOM_USB_PIN_DM
#define CUSTOM_USB_PIN_DM 4
#endif

#define USB_PORT   CUSTOM_USB_PORT
#define USB_PIN_DP CUSTOM_USB_PIN_DP
#define USB_PIN_DM CUSTOM_USB_PIN_DM
#ifdef CUSTOM_USB_PIN_DPU
#define USB_PIN_DPU CUSTOM_USB_PIN_DPU
#endif

/* rv003usb's receive loop encodes the D+/D- pin numbers in a c.andi immediate,
 * which only holds 5 bits - so pins 5..7 of any port cannot carry USB. Catch
 * that here rather than at "why does it not enumerate". */
#if (CUSTOM_USB_PIN_DP) > 4 || (CUSTOM_USB_PIN_DM) > 4
#error "CUSTOM_USB_PIN_DP / _DM must be pin 0-4: rv003usb cannot bit-bang USB \
on pins 5-7. Valid combinations on a CH32V003 are within PC0-PC4 or PD0-PD4."
#endif

#define RV003USB_OPTIMIZE_FLASH    1
#define RV003USB_EVENT_DEBUGGING   0
#define RV003USB_HANDLE_IN_REQUEST 1 /* we drive EP1/EP2 IN ourselves       */
#define RV003USB_OTHER_CONTROL     1 /* needed to observe SET_CONFIGURATION */
#define RV003USB_HANDLE_USER_DATA  1 /* EP2 OUT carries VIA commands        */
#define RV003USB_HID_FEATURES      0

#ifndef CUSTOM_VID
#define CUSTOM_VID 0x1209
#endif
#ifndef CUSTOM_PID
#define CUSTOM_PID 0xB803
#endif

#ifndef STR_MANUFACTURER
#define STR_MANUFACTURER u"UMK"
#endif
#ifndef STR_PRODUCT
#define STR_PRODUCT u"UMK_VIA"
#endif
#ifndef STR_SERIAL
#define STR_SERIAL u"001"
#endif

#ifndef __ASSEMBLER__

#include <stdint.h>

/* USB_PORT is a bare port letter and USB_PIN_* are bare pin numbers, because
 * that is the shape rv003usb wants. Paste them into the PxN constants that
 * funPinMode/funDigitalWrite take. */
#define UMK_PIN_CONCAT_(port, pin) P##port##pin
#define UMK_PIN_CONCAT(port, pin)  UMK_PIN_CONCAT_(port, pin)
#define UMK_USB_PIN_DM             UMK_PIN_CONCAT(USB_PORT, USB_PIN_DM)

/* Set once the host issues SET_CONFIGURATION. Split role detection in main.c
 * uses this to tell the USB-connected half from the far half. */
extern volatile uint8_t usb_configured_flag;

/* Makes the host register a disconnect before we go on-bus. Call before
 * usb_setup(). */
void usb_force_reenumerate(void);

/* 8 byte HID Boot Keyboard report, refilled by the main loop and shipped
 * straight out of the EP1 IN interrupt. */
extern uint8_t hid_keyboard_report[8];

#ifdef INSTANCE_DESCRIPTORS

static const uint8_t device_descriptor[] = {
	18,                     /* bLength            */
	1,                      /* bDescriptorType    */
	0x10, 0x01,             /* bcdUSB 1.1         */
	0x00,                   /* bDeviceClass       */
	0x00,                   /* bDeviceSubClass    */
	0x00,                   /* bDeviceProtocol    */
	0x08,                   /* bMaxPacketSize0 - 8 is mandatory for Low-Speed */
	(CUSTOM_VID) & 0xFF, ((CUSTOM_VID) >> 8) & 0xFF,
	(CUSTOM_PID) & 0xFF, ((CUSTOM_PID) >> 8) & 0xFF,
	0x01, 0x00,             /* bcdDevice          */
	1,                      /* iManufacturer      */
	2,                      /* iProduct           */
	3,                      /* iSerialNumber      */
	1,                      /* bNumConfigurations */
};

/* Standard 6KRO boot keyboard: modifier byte, reserved byte, 6 keycodes. */
static const uint8_t keyboard_hid_desc[] = {
	0x05, 0x01,       /* Usage Page (Generic Desktop)     */
	0x09, 0x06,       /* Usage (Keyboard)                 */
	0xA1, 0x01,       /* Collection (Application)         */
	0x05, 0x07,       /*   Usage Page (Keyboard)          */
	0x19, 0xE0,       /*   Usage Minimum (LeftControl)    */
	0x29, 0xE7,       /*   Usage Maximum (Right GUI)      */
	0x15, 0x00,       /*   Logical Minimum (0)            */
	0x25, 0x01,       /*   Logical Maximum (1)            */
	0x75, 0x01,       /*   Report Size (1)                */
	0x95, 0x08,       /*   Report Count (8)               */
	0x81, 0x02,       /*   Input (Data,Var,Abs) modifiers */
	0x95, 0x01,       /*   Report Count (1)               */
	0x75, 0x08,       /*   Report Size (8)                */
	0x81, 0x03,       /*   Input (Cnst,Var,Abs) reserved  */
	0x95, 0x05,       /*   Report Count (5)               */
	0x75, 0x01,       /*   Report Size (1)                */
	0x05, 0x08,       /*   Usage Page (LEDs)              */
	0x19, 0x01,       /*   Usage Minimum (Num Lock)       */
	0x29, 0x05,       /*   Usage Maximum (Kana)           */
	0x91, 0x02,       /*   Output (Data,Var,Abs) LEDs     */
	0x95, 0x01,       /*   Report Count (1)               */
	0x75, 0x03,       /*   Report Size (3)                */
	0x91, 0x03,       /*   Output (Cnst,Var,Abs) padding  */
	0x95, 0x06,       /*   Report Count (6)               */
	0x75, 0x08,       /*   Report Size (8)                */
	0x15, 0x00,       /*   Logical Minimum (0)            */
	0x25, 0xA7,       /*   Logical Maximum (167)          */
	0x05, 0x07,       /*   Usage Page (Keyboard)          */
	0x19, 0x00,       /*   Usage Minimum (0)              */
	0x29, 0xA7,       /*   Usage Maximum (167)            */
	0x81, 0x00,       /*   Input (Data,Ary,Abs) keys      */
	0xC0,             /* End Collection                   */
};

/* VIA raw HID. 32 byte reports, split across 8 byte Low-Speed packets. */
static const uint8_t raw_hid_desc[] = {
	0x06, 0x60, 0xFF, /* Usage Page (Vendor Defined 0xFF60) */
	0x09, 0x61,       /* Usage (0x61)                       */
	0xA1, 0x01,       /* Collection (Application)           */
	0x09, 0x62,       /*   Usage (0x62)                     */
	0x15, 0x00,       /*   Logical Minimum (0)              */
	0x26, 0xFF, 0x00, /*   Logical Maximum (255)            */
	0x95, 0x20,       /*   Report Count (32)                */
	0x75, 0x08,       /*   Report Size (8)                  */
	0x81, 0x02,       /*   Input (Data,Var,Abs)             */
	0x09, 0x63,       /*   Usage (0x63)                     */
	0x15, 0x00,       /*   Logical Minimum (0)              */
	0x26, 0xFF, 0x00, /*   Logical Maximum (255)            */
	0x95, 0x20,       /*   Report Count (32)                */
	0x75, 0x08,       /*   Report Size (8)                  */
	0x91, 0x02,       /*   Output (Data,Var,Abs)            */
	0xC0,             /* End Collection                     */
};

#define UMK_CONFIG_TOTAL_LEN (9 + (9 + 9 + 7) + (9 + 9 + 7 + 7))

static const uint8_t config_descriptor[] = {
	/* Configuration */
	9, 2,
	UMK_CONFIG_TOTAL_LEN & 0xFF, (UMK_CONFIG_TOTAL_LEN >> 8) & 0xFF,
	0x02,                   /* bNumInterfaces    */
	0x01,                   /* bConfigurationValue */
	0x00,                   /* iConfiguration    */
	0x80,                   /* bmAttributes: bus powered */
	0x32,                   /* bMaxPower 100mA   */

	/* Interface 0 - Boot Keyboard */
	9, 4,
	0x00,                   /* bInterfaceNumber  */
	0x00,                   /* bAlternateSetting */
	0x01,                   /* bNumEndpoints     */
	0x03,                   /* bInterfaceClass: HID       */
	0x01,                   /* bInterfaceSubClass: Boot   */
	0x01,                   /* bInterfaceProtocol: Keyboard */
	0x00,                   /* iInterface        */

	9, 0x21,
	0x10, 0x01,             /* bcdHID 1.1        */
	0x00,                   /* bCountryCode      */
	0x01,                   /* bNumDescriptors   */
	0x22,                   /* bDescriptorType: Report */
	sizeof(keyboard_hid_desc) & 0xFF, (sizeof(keyboard_hid_desc) >> 8) & 0xFF,

	7, 0x05,
	0x81,                   /* EP1 IN            */
	0x03,                   /* Interrupt         */
	0x08, 0x00,             /* wMaxPacketSize 8  */
	10,                     /* bInterval 10ms - the Low-Speed floor */

	/* Interface 1 - VIA Raw HID */
	9, 4,
	0x01,                   /* bInterfaceNumber  */
	0x00,                   /* bAlternateSetting */
	0x02,                   /* bNumEndpoints     */
	0x03,                   /* bInterfaceClass: HID */
	0x00,                   /* bInterfaceSubClass: none */
	0x00,                   /* bInterfaceProtocol: none */
	0x00,                   /* iInterface        */

	9, 0x21,
	0x10, 0x01,
	0x00,
	0x01,
	0x22,
	sizeof(raw_hid_desc) & 0xFF, (sizeof(raw_hid_desc) >> 8) & 0xFF,

	7, 0x05,
	0x82,                   /* EP2 IN            */
	0x03,
	0x08, 0x00,
	10,

	7, 0x05,
	0x02,                   /* EP2 OUT           */
	0x03,
	0x08, 0x00,
	10,
};

struct usb_string_descriptor_struct {
	uint8_t  bLength;
	uint8_t  bDescriptorType;
	uint16_t wString[];
};

const static struct usb_string_descriptor_struct string0 __attribute__((section(".rodata"))) = {
	4, 3, { 0x0409 }
};
const static struct usb_string_descriptor_struct string1 __attribute__((section(".rodata"))) = {
	sizeof(STR_MANUFACTURER), 3, STR_MANUFACTURER
};
const static struct usb_string_descriptor_struct string2 __attribute__((section(".rodata"))) = {
	sizeof(STR_PRODUCT), 3, STR_PRODUCT
};
const static struct usb_string_descriptor_struct string3 __attribute__((section(".rodata"))) = {
	sizeof(STR_SERIAL), 3, STR_SERIAL
};

/* lIndexValue packs wValue in the low half and wIndex in the high half, which
 * is how the two report descriptors get told apart by interface number. */
const static struct descriptor_list_struct {
	uint32_t      lIndexValue;
	const uint8_t *addr;
	uint8_t       length;
} descriptor_list[] = {
	{ 0x00000100, device_descriptor,          sizeof(device_descriptor) },
	{ 0x00000200, config_descriptor,          sizeof(config_descriptor) },
	{ 0x00002200, keyboard_hid_desc,          sizeof(keyboard_hid_desc) },
	{ 0x00012200, raw_hid_desc,               sizeof(raw_hid_desc)      },
	{ 0x00000300, (const uint8_t *)&string0,  4                         },
	{ 0x04090301, (const uint8_t *)&string1,  sizeof(STR_MANUFACTURER)  },
	{ 0x04090302, (const uint8_t *)&string2,  sizeof(STR_PRODUCT)       },
	{ 0x04090303, (const uint8_t *)&string3,  sizeof(STR_SERIAL)        },
};
#define DESCRIPTOR_LIST_ENTRIES ((sizeof(descriptor_list)) / (sizeof(struct descriptor_list_struct)))

#endif /* INSTANCE_DESCRIPTORS */

#endif /* __ASSEMBLER__ */

#endif
