ifeq ($(HEAP_SIZE),)
HEAP_SIZE      = 8388208
endif
ifeq ($(STACK_SIZE),)
STACK_SIZE     = 61800
endif

ifeq ($(PRODUCT),)
PRODUCT = peanutGB_Playdate.pdx
endif

SELF_DIR := $(dir $(lastword $(MAKEFILE_LIST)))


SDK = ${PLAYDATE_SDK_PATH}
ifeq ($(SDK),)
SDK = $(shell egrep '^\s*SDKRoot' ~/.Playdate/config | head -n 1 | cut -c9-)
endif

ifeq ($(SDK),)
$(error SDK path not found; set ENV value PLAYDATE_SDK_PATH)
endif

VPATH += $(SELF_DIR)/src $(SELF_DIR)/playdate-coroutines $(SELF_DIR)/playdate-cpp

# List C source files here
SRC += src/main.c \
	src/rom_list.c \
	src/minigb_apu/minigb_apu.c

# List all user directories here
UINCDIR += $(VPATH)

# List all user C define here, like -D_DEBUG=1
UDEFS += -DENABLE_SOUND -DENABLE_SOUND_MINIGB -Wno-strict-prototypes

# Define ASM defines here
UADEFS +=

# List the user directory to look for the libraries here
ULIBDIR +=

# List all user libraries here
ULIBS +=

CLANGFLAGS += -fsingle-precision-constant -Wdouble-promotion

include $(SDK)/C_API/buildsupport/common.mk