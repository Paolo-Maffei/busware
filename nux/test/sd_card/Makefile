#/*
#    nux V1.0
#	
#*/

SFLASH=sflash
TTY=/dev/tty.SLAB_USBtoUART
RTOS_SOURCE_DIR=../../FreeRTOS/Source
LUMINARY_DRIVER_DIR=../../LuminaryMicro

CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy
LDSCRIPT=standalone.ld

LINKER_FLAGS=-nostartfiles -Xlinker -osdtest.axf -Xlinker -M -Xlinker -Map=sdtest.map -Xlinker --gc-sections --entry ResetISR

IPATH=../..
IPATH+=../../utils
IPATH+=$(RTOS_SOURCE_DIR)/include
IPATH+=$(RTOS_SOURCE_DIR)/portable/GCC/ARM_CM3
IPATH+=$(LUMINARY_DRIVER_DIR)

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
		../../utils/vstdlib.c \
		../../utils/cmdline.c \
		../../fatfs/src/ff.c \
		../../fatfs/port/nux4a.c \
		$(RTOS_SOURCE_DIR)/list.c \
		$(RTOS_SOURCE_DIR)/queue.c \
		$(RTOS_SOURCE_DIR)/tasks.c \
		$(RTOS_SOURCE_DIR)/portable/GCC/ARM_CM3/port.c \
		$(RTOS_SOURCE_DIR)/portable/MemMang/heap_3.c 

LIBS= $(LUMINARY_DRIVER_DIR)/arm-none-eabi-gcc/libdriver.a

OBJS = $(SOURCE:.c=.o)


#
# Add the include file paths to CFLAGS.
#
CFLAGS+=${patsubst %,-I%,${subst :, ,${IPATH}}}

.PHONY: all flash clean
all: sdtest.bin
	 

flash: all
	$(SFLASH) -c $(TTY) -b 115200 -p 0x1000 -d sdtest.bin

sdtest.bin : sdtest.axf
	$(OBJCOPY) sdtest.axf -O binary sdtest.bin

sdtest.axf : $(OBJS) Makefile
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) $(LINKER_FLAGS)

$(OBJS) : %.o : %.c Makefile FreeRTOSConfig.h
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	touch Makefile
	rm -f $(OBJS)
	rm  -f *.map
	rm  -f *.bin
	rm  -f *.axf
	rm  -f *.elf

