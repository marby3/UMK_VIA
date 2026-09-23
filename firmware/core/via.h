/* UMK_VIA - VIA protocol (version 0x000C).
 *
 * Work is split between the USB interrupt and the main loop because rv003usb
 * ACKs the host the instant the interrupt handler returns; anything slow in
 * there breaks the Low-Speed turnaround window. See spec 4.5.
 */
#ifndef _UMK_VIA_H
#define _UMK_VIA_H

#include <stdint.h>

/* Fixed by Remap's implementation - do not bump. */
#define VIA_PROTOCOL_VERSION 0x000C

#ifndef VIA_FIRMWARE_VERSION
#define VIA_FIRMWARE_VERSION 0x00000001
#endif

#define VIA_PACKET_SIZE 32

struct usb_endpoint;

void via_init(void);

/* Main loop: interprets a completed command and prepares the reply. */
void via_task(void);

/* EP2 OUT interrupt: accumulates 8 byte chunks into the 32 byte command. */
void via_receive_packet(uint8_t *data, int len);

/* EP2 IN interrupt: ships the prepared reply 8 bytes at a time, or an empty
 * packet when nothing is pending (Remap disconnects on unsolicited reports). */
void via_handle_in(struct usb_endpoint *e, uint32_t sendtok);

#endif
