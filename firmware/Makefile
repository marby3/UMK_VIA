all : flash

TARGET:=main
CH32FUN:=ch32v003fun/ch32fun
TARGET_MCU:=CH32V003

# Check KEYBOARD
ifndef KEYBOARD
$(error KEYBOARD is not defined. Usage: make KEYBOARD=<keyboard_name>)
endif

KEYBOARD_DIR := ../keyboards/$(KEYBOARD)

# Include keyboard specific rules
-include $(KEYBOARD_DIR)/rules.mk

# Optimize for size
EXTRA_CFLAGS:=-Os -Irv003usb/rv003usb -Irv003usb/lib -I$(KEYBOARD_DIR)

ADDITIONAL_C_FILES+=rv003usb/rv003usb/rv003usb.S rv003usb/rv003usb/rv003usb.c usb_config.c matrix.c split.c rgb_led.c keymap.c flash_store.c via.c

# Include keyboard specific keymap.c if exists
ifneq ("$(wildcard $(KEYBOARD_DIR)/keymap.c)","")
    ADDITIONAL_C_FILES += $(KEYBOARD_DIR)/keymap.c
endif

include $(CH32FUN)/ch32fun.mk

flash : cv_flash
clean : cv_clean
