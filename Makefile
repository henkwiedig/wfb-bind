# Get the current commit hash (shortened to 7 characters)
GIT_COMMIT := $(shell git rev-parse --short HEAD)
# Check if there are uncommitted changes (either staged or unstaged)
GIT_DIRTY := $(shell (git diff --quiet && git diff --cached --quiet) || echo "-dirty")
# Combine commit hash and dirty marker
VERSION_STRING := $(GIT_COMMIT)$(GIT_DIRTY)

CFLAGS ?=
CFLAGS += -DVERSION_STRING="\"$(VERSION_STRING)\""
SRCS := wfb_bind.c
OUTPUT ?= $(PWD)
BUILD = $(CC) $(SRCS) -I$(TOOLCHAIN)/usr/include $(CFLAGS) -o $(OUTPUT)

clean:
	rm -f *.o wfb_bind_x86 wfb_bind_goke wfb_bind_hisi wfb_bind_star6b0 wfb_bind_star6e wfb_bind_rockchip

goke:
	$(BUILD)

hisi:
	$(BUILD)

star6b0:
	$(BUILD)

star6e:
	$(BUILD)

x86:
	$(BUILD)

rockchip:
	$(BUILD)	