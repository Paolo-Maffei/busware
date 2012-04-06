#/*
#    nux V1.0
#	
#*/

SFLASH=sflash
TTY=/dev/tty.SLAB_USBtoUART
LUMINARY_DRIVER_DIR=LuminaryMicro

CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy
LDSCRIPT=standalone.ld

SRCHOME=../..

LINKER_FLAGS=-nostartfiles -Xlinker -otest.axf -Xlinker -M -Xlinker -Map=test.map -Xlinker --gc-sections --entry ResetISR

IPATH=.
IPATH+=$(SRCHOME)
IPATH+=$(SRCHOME)/$(LUMINARY_DRIVER_DIR)

CFLAGS= -std=c99                                   \
		-Wall                                      \
		-D GCC_ARMCM3                              \
		-D PART_LM3S611                            \
		-D inline=                                 \
		-mthumb                                    \
		-mcpu=cortex-m3                            \
		-Os                                        \
		-T$(LDSCRIPT)                              \
		-D sprintf=usprintf                        \
		-D snprintf=usnprintf                      \
		-D printf=uipprintf                        \
		-ffunction-sections                        \
		-fdata-sections                            \
		-g -D DEBUG

SOURCE=	main.c \
		startup.c \
		uart.c \
		$(SRCHOME)/utils/vstdlib.c

LIBS= $(SRCHOME)/$(LUMINARY_DRIVER_DIR)/arm-none-eabi-gcc/libdriver.a

OBJS = $(SOURCE:.c=.o)


#
# Add the include file paths to CFLAGS.
#
CFLAGS+=${patsubst %,-I%,${subst :, ,${IPATH}}}

.PHONY: all flash clean
all: test.bin

flash: all
	$(SFLASH) -c $(TTY) -b 115200 -p 0x1000 -d test.bin

test.bin : test.axf
	$(OBJCOPY) test.axf -O binary test.bin

test.axf : $(OBJS) Makefile
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) $(LINKER_FLAGS)

$(OBJS) : %.o : %.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	touch Makefile
	rm -f $(OBJS)
	rm  -f *.map
	rm  -f *.bin
	rm  -f *.axf
	rm  -f *.elf
