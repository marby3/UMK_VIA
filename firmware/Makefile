all : flash

TARGET:=main
CH32FUN:=ch32v003fun/ch32fun
TARGET_MCU:=CH32V003

# Optimize for size
EXTRA_CFLAGS:=-Os -Irv003usb/rv003usb -Irv003usb/lib

ADDITIONAL_C_FILES+=rv003usb/rv003usb/rv003usb.S rv003usb/rv003usb/rv003usb.c usb_config.c matrix.c keymap.c flash_store.c

include $(CH32FUN)/ch32fun.mk

flash : cv_flash
clean : cv_clean
