# uiapduino - keyboard specific build settings.
#
# Included by firmware/Makefile before ch32fun.mk, so anything set here lands
# in the real build. Feature toggles themselves live in config.h; this file is
# for build flags only.

# Example: trade a little speed for flash on a nearly-full build.
# EXTRA_CFLAGS += -Os
