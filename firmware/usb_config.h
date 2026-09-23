#ifndef _USB_CONFIG_H
#define _USB_CONFIG_H

// 3 endpoints: EP0 (Control), EP1 (Keyboard IN), EP2 (VIA Raw HID IN/OUT)
#define ENDPOINTS 3

// VIA/Remap が要求する Raw HID のレポートサイズ (固定 32 バイト)。
// rv003usb は Low-Speed USB のため 1 パケットは最大 8 バイトに制限されます。
// 32 バイトのレポートは 8 バイト x 4 パケットのマルチパケット転送として
// via.c 側で組み立て・分割しています。
#define VIA_RAW_EPSIZE 32

#define USB_PORT D
#define USB_PIN_DP 3
#define USB_PIN_DM 4
#define USB_PIN_DPU 5

#define RV003USB_OPTIMIZE_FLASH 1
#define RV003USB_EVENT_DEBUGGING 0
#define RV003USB_HANDLE_IN_REQUEST 1
#define RV003USB_OTHER_CONTROL 0
#define RV003USB_HANDLE_USER_DATA 1
#define RV003USB_HID_FEATURES 0

#ifndef __ASSEMBLER__

#if __has_include("config.h")
#include "config.h"
#endif

#ifndef CUSTOM_VID
#define CUSTOM_VID 0x1209
#endif

#ifndef CUSTOM_PID
#define CUSTOM_PID 0xB803
#endif

#include <tinyusb_hid.h>

#ifdef INSTANCE_DESCRIPTORS

static const uint8_t device_descriptor[] = {
	18, //Length
	1,  //Type (Device)
	0x10, 0x01, //Spec (USB 1.1)
	0x0, //Device Class
	0x0, //Device Subclass
	0x0, //Device Protocol
	0x08, //Max packet size for EP0 (8 bytes)
	(CUSTOM_VID & 0xFF), ((CUSTOM_VID >> 8) & 0xFF), //ID Vendor
	(CUSTOM_PID & 0xFF), ((CUSTOM_PID >> 8) & 0xFF), //ID Product
	0x02, 0x00, //ID Rev
	1, //Manufacturer string
	2, //Product string
	3, //Serial string
	1, //Max number of configurations
};

// Standard Boot Keyboard
static const uint8_t keyboard_hid_desc[] = {
	HID_USAGE_PAGE( HID_USAGE_PAGE_DESKTOP ),
	HID_USAGE( HID_USAGE_DESKTOP_KEYBOARD ),
	HID_COLLECTION ( HID_COLLECTION_APPLICATION ),
		HID_REPORT_SIZE( 1 ),
		HID_REPORT_COUNT( 8 ),
		HID_USAGE_PAGE( HID_USAGE_PAGE_KEYBOARD ),
    	HID_USAGE_MIN( 0xe0 ),
    	HID_USAGE_MAX( 0xe7 ),
		HID_LOGICAL_MIN( 0 ),
		HID_LOGICAL_MAX( 1 ),
		HID_INPUT( 0x02 ), // Modifier byte
		HID_REPORT_COUNT( 1 ),
		HID_REPORT_SIZE( 8 ),
		HID_INPUT( 0x03 ), // Reserved byte
		HID_REPORT_COUNT( 5 ),
		HID_REPORT_SIZE( 1 ),
		HID_USAGE_PAGE( HID_USAGE_PAGE_LED ),
    	HID_USAGE_MIN( 0x01 ),
	    HID_USAGE_MAX( 0x05 ),
		HID_OUTPUT( 0x02 ), // LED report
		HID_REPORT_COUNT( 1 ),
		HID_REPORT_SIZE( 3 ),
		HID_OUTPUT( 0x03 ), // LED report padding
		HID_REPORT_COUNT( 6 ),
		HID_REPORT_SIZE( 8 ),
		HID_LOGICAL_MIN( 0 ),
		HID_LOGICAL_MAX( 101 ),
    	HID_USAGE_PAGE( HID_USAGE_PAGE_KEYBOARD ),
    	HID_USAGE_MIN( 0x00 ),
	    HID_USAGE_MAX( 101 ),
	HID_INPUT( 0 ), // Key array (6 keys)
    HID_COLLECTION_END,
};

// VIA / Remap Raw HID (32 bytes IN, 32 bytes OUT)
// Usage Page 0xFF60 / Usage 0x61 は VIA の必須要件です。
// Remap はこの Usage Page/Usage を持つコレクションのみをキーボードとして認識します。
// (remap/src/services/hid/WebHid.ts: usagePage === 0xff60 && usage === 0x61)
static const uint8_t custom_hid_desc[] = {
    0x06, 0x60, 0xFF,  // Usage Page (Vendor Defined 0xFF60)
    0x09, 0x61,        // Usage (0x61)
    0xA1, 0x01,        // Collection (Application)
        0x15, 0x00,    //   Logical Minimum (0)
        0x26, 0xFF,0x00, // Logical Maximum (255)
        0x75, 0x08,    //   Report Size (8 bits)
        0x95, VIA_RAW_EPSIZE, //   Report Count (32 bytes)
        0x09, 0x62,    //   Usage (0x62)
        0x81, 0x02,    //   Input (Data, Var, Abs)
        0x95, VIA_RAW_EPSIZE, //   Report Count (32 bytes)
        0x09, 0x63,    //   Usage (0x63)
        0x91, 0x02,    //   Output (Data, Var, Abs)
    0xC0               // End Collection
};

static const uint8_t config_descriptor[] = {
	9, // bLength;
	2, // bDescriptorType;
	0x42, 0x00, // wTotalLength (9+9+9+7 + 9+9+7+7 = 66 => 0x42)
	0x02, // bNumInterfaces (2: Keyboard and Custom)
	0x01, // bConfigurationValue
	0x00, // iConfiguration
	0x80, // bmAttributes
	0x64, // bMaxPower (200mA)

	// Keyboard Interface (Interface 0)
	9, // bLength
	4, // bDescriptorType
	0, // bInterfaceNumber
	0, // bAlternateSetting
	1, // bNumEndpoints
	0x03, // bInterfaceClass (HID)
	0x01, // bInterfaceSubClass (Boot Interface)
	0x01, // bInterfaceProtocol (Keyboard)
	0, // iInterface

	9, // bLength
	0x21, // bDescriptorType (HID)
	0x11,0x01, // bcd 1.11
	0x00, // country code
	0x01, // Num descriptors
	0x22, // DescriptorType (HID)
	sizeof(keyboard_hid_desc), 0x00, 

	7, // endpoint descriptor (EP 1 IN)
	0x05, // bDescriptorType (Endpoint)
	0x81, // bEndpointAddress (EP1 IN)
	0x03, // bmAttributes (Interrupt)
	0x08, 0x00, // wMaxPacketSize (8 bytes)
	10, // bInterval (10ms)

	// VIA Raw HID Interface (Interface 1)
	9, // bLength
	4, // bDescriptorType
	1, // bInterfaceNumber
	0, // bAlternateSetting
	2, // bNumEndpoints (IN and OUT)
	0x03, // bInterfaceClass (HID)
	0x00, // bInterfaceSubClass (None)
	0x00, // bInterfaceProtocol (None)
	0, // iInterface

	9, // bLength
	0x21, // bDescriptorType (HID)
	0x11,0x01, // bcd 1.11
	0x00, // country code
	0x01, // Num descriptors
	0x22, // DescriptorType (HID)
	sizeof(custom_hid_desc), 0x00, 

	// 注: Low-Speed USB では wMaxPacketSize の上限が 8 バイト、bInterval の下限が
	// 10ms と規格で決まっています。VIA の 32 バイトレポートはホスト側が
	// 8 バイト x 4 トランザクションに分割して転送します。
	7, // endpoint descriptor (EP 2 IN)
	0x05, // bDescriptorType (Endpoint)
	0x82, // bEndpointAddress (EP2 IN)
	0x03, // bmAttributes (Interrupt)
	0x08, 0x00, // wMaxPacketSize (8 bytes / Low-Speed 上限)
	10, // bInterval (10ms)

	7, // endpoint descriptor (EP 2 OUT)
	0x05, // bDescriptorType (Endpoint)
	0x02, // bEndpointAddress (EP2 OUT)
	0x03, // bmAttributes (Interrupt)
	0x08, 0x00, // wMaxPacketSize (8 bytes / Low-Speed 上限)
	10, // bInterval (10ms)
};

#ifndef STR_MANUFACTURER
#define STR_MANUFACTURER u"UIAPduino"
#endif
#ifndef STR_PRODUCT
#define STR_PRODUCT      u"UIAPduino_VIA"
#endif
#ifndef STR_SERIAL
#define STR_SERIAL       u"001"
#endif

// String descriptors
struct usb_string_descriptor_struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wString[];
};
const static struct usb_string_descriptor_struct string0 __attribute__((section(".rodata"))) = {
	4, 3, {0x0409}
};
const static struct usb_string_descriptor_struct string1 __attribute__((section(".rodata")))  = {
	sizeof(STR_MANUFACTURER), 3, STR_MANUFACTURER
};
const static struct usb_string_descriptor_struct string2 __attribute__((section(".rodata")))  = {
	sizeof(STR_PRODUCT), 3, STR_PRODUCT
};
const static struct usb_string_descriptor_struct string3 __attribute__((section(".rodata")))  = {
	sizeof(STR_SERIAL), 3, STR_SERIAL
};

const static struct descriptor_list_struct {
	uint32_t	lIndexValue;
	const uint8_t	*addr;
	uint8_t		length;
} descriptor_list[] = {
	{0x00000100, device_descriptor, sizeof(device_descriptor)},
	{0x00000200, config_descriptor, sizeof(config_descriptor)},
	{0x00002200, keyboard_hid_desc, sizeof(keyboard_hid_desc)},
	{0x00012200, custom_hid_desc, sizeof(custom_hid_desc)},
	{0x00000300, (const uint8_t *)&string0, 4},
	{0x04090301, (const uint8_t *)&string1, sizeof(STR_MANUFACTURER)},
	{0x04090302, (const uint8_t *)&string2, sizeof(STR_PRODUCT)},	
	{0x04090303, (const uint8_t *)&string3, sizeof(STR_SERIAL)}
};
#define DESCRIPTOR_LIST_ENTRIES ((sizeof(descriptor_list))/(sizeof(struct descriptor_list_struct)) )

#endif // INSTANCE_DESCRIPTORS

#endif // __ASSEMBLER__

#endif 
