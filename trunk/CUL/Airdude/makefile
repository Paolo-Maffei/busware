#
#             LUFA Library
#     Copyright (C) Dean Camera, 2013.
#
#  dean [at] fourwalledcubicle [dot] com
#           www.lufa-lib.org
#
# --------------------------------------
#         LUFA Project Makefile.
# --------------------------------------

# Run "make help" for target help.

MCU          = atmega32u4
ARCH         = AVR8
BOARD        = CULV3
F_CPU        = 8000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
TARGET       = Airdude
SRC          = $(TARGET).c Descriptors.c $(LUFA_SRC_USB) $(LUFA_SRC_USBCLASS)
SRC         += ../../lib/seriallink/seriallink.c ../../lib/seriallink/ringbuf.c
SRC	    += ../../lib/crypto/rsa_dec.S  ../../lib/crypto/rsa_enc.S   ../../lib/crypto/RSA_MODULO.S ../../lib/crypto/rsa_mul.S
SRC	    += ../../lib/crypto/rsa_decrypt.c ../../lib/crypto/rsa_encrypt.c
SRC         += ../../lib/crypto-lib/aes/aes_dec.c ../../lib/crypto-lib/aes/aes_keyschedule.c ../../lib/crypto-lib/aes/aes_enc.c ../../lib/crypto-lib/aes/aes_sbox.c ../../lib/crypto-lib/aes/aes_invsbox.c ../../lib/crypto-lib/gf256mul/gf256mul.S
SRC         += ../../lib/crypto-lib/aes/aes128_enc.c ../../lib/crypto-lib/aes/aes128_dec.c

LUFA_PATH    = ../../lufa-lib-read-only/LUFA
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -IConfig/ -I../../lib/seriallink/ -I../../lib/crypto/ -I../../lib/crypto-lib/gf256mul/ -I../../lib/crypto-lib/aes/
CC_FLAGS    += -DRADIO_CRYPT
LD_FLAGS     =

# Default target
all:

# Include LUFA build script makefiles
include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_cppcheck.mk
include $(LUFA_PATH)/Build/lufa_doxygen.mk
include $(LUFA_PATH)/Build/lufa_dfu.mk
include $(LUFA_PATH)/Build/lufa_hid.mk
include $(LUFA_PATH)/Build/lufa_avrdude.mk
include $(LUFA_PATH)/Build/lufa_atprogram.mk
