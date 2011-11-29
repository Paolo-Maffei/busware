#/*
#    nux V1.0
#	
#*/

RTOS_SOURCE_DIR=FreeRTOS/Source
LWIP_SOURCE_DIR=lwip-1.3.2
LUMINARY_DRIVER_DIR=FreeRTOS/Common/drivers/LuminaryMicro

CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy
LDSCRIPT=standalone.ld

# should use --gc-sections but the debugger does not seem to be able to cope with the option.
LINKER_FLAGS=-nostartfiles -Xlinker -onux.axf -Xlinker -M -Xlinker -Map=nux.map -Xlinker --no-gc-sections

DEBUG=-g
OPTIM=-Os


CFLAGS=$(DEBUG) -I . -I utils -I softeeprom -I $(RTOS_SOURCE_DIR)/include -I $(RTOS_SOURCE_DIR)/portable/GCC/ARM_CM3 \
		-D GCC_ARMCM3 -D inline= -mthumb -mcpu=cortex-m3 $(OPTIM) -T$(LDSCRIPT) \
		-D sprintf=usprintf -D snprintf=usnprintf -D printf=uipprintf -I $(LWIP_SOURCE_DIR) \
		-I $(LWIP_SOURCE_DIR)/src/include/ipv4 -I $(LWIP_SOURCE_DIR)/src/include -I $(LWIP_SOURCE_DIR)/contrib/port/FreeRTOS/LM3S \
		-ffunction-sections -fdata-sections -I $(LUMINARY_DRIVER_DIR)

SOURCE=	main.c \
		uart.c \
		console.c \
		softeeprom/softeeprom.c \
		utils/ustdlib.c \
		utils/cmdline.c \
		ETHIsr.c \
		LWIPStack.c \
		telnetd.c \
		rawuart.c \
		$(RTOS_SOURCE_DIR)/list.c \
		$(RTOS_SOURCE_DIR)/queue.c \
		$(RTOS_SOURCE_DIR)/tasks.c \
		$(RTOS_SOURCE_DIR)/portable/GCC/ARM_CM3/port.c \
		$(RTOS_SOURCE_DIR)/portable/MemMang/heap_3.c \
		$(LWIP_SOURCE_DIR)/contrib/port/FreeRTOS/sys_arch.c \
		$(LWIP_SOURCE_DIR)/src/api/api_lib.c \
		$(LWIP_SOURCE_DIR)/src/api/api_msg.c \
		$(LWIP_SOURCE_DIR)/src/api/tcpip.c \
		$(LWIP_SOURCE_DIR)/src/api/err.c \
		$(LWIP_SOURCE_DIR)/src/api/sockets.c \
		$(LWIP_SOURCE_DIR)/src/api/netbuf.c \
		$(LWIP_SOURCE_DIR)/src/api/netdb.c \
		$(LWIP_SOURCE_DIR)/src/api/netifapi.c \
		$(LWIP_SOURCE_DIR)/src/core/dhcp.c \
		$(LWIP_SOURCE_DIR)/src/core/dns.c \
		$(LWIP_SOURCE_DIR)/src/core/init.c \
		$(LWIP_SOURCE_DIR)/src/core/mem.c \
		$(LWIP_SOURCE_DIR)/src/core/memp.c \
		$(LWIP_SOURCE_DIR)/src/core/netif.c \
		$(LWIP_SOURCE_DIR)/src/core/pbuf.c \
		$(LWIP_SOURCE_DIR)/src/core/raw.c \
		$(LWIP_SOURCE_DIR)/src/core/stats.c \
		$(LWIP_SOURCE_DIR)/src/core/sys.c \
		$(LWIP_SOURCE_DIR)/src/core/tcp.c \
		$(LWIP_SOURCE_DIR)/src/core/tcp_in.c \
		$(LWIP_SOURCE_DIR)/src/core/tcp_out.c \
		$(LWIP_SOURCE_DIR)/src/core/udp.c \
		$(LWIP_SOURCE_DIR)/src/core/ipv4/autoip.c \
		$(LWIP_SOURCE_DIR)/src/core/ipv4/icmp.c \
		$(LWIP_SOURCE_DIR)/src/core/ipv4/igmp.c \
		$(LWIP_SOURCE_DIR)/src/core/ipv4/inet.c \
		$(LWIP_SOURCE_DIR)/src/core/ipv4/inet_chksum.c \
		$(LWIP_SOURCE_DIR)/src/core/ipv4/ip_addr.c \
		$(LWIP_SOURCE_DIR)/src/core/ipv4/ip.c \
		$(LWIP_SOURCE_DIR)/src/core/ipv4/ip_frag.c

		# NETIFFILES: Files implementing various generic network interface functions.
NETIFFILES=$(LWIP_SOURCE_DIR)/src/netif/etharp.c \
			$(LWIP_SOURCE_DIR)/src/netif/loopif.c

LIBS= $(LUMINARY_DRIVER_DIR)/arm-none-eabi-gcc/libdriver.a $(LUMINARY_DRIVER_DIR)/arm-none-eabi-gcc/libgr.a

OBJS = $(SOURCE:.c=.o) $(NETIFFILES:.c=.o) 

all: nux.bin
	 
nux.bin : nux.axf
	$(OBJCOPY) nux.axf -O binary nux.bin

nux.axf : $(OBJS) startup.o Makefile
	$(CC) $(CFLAGS) $(OBJS) startup.o $(LIBS) $(LINKER_FLAGS)

$(OBJS) : %.o : %.c Makefile FreeRTOSConfig.h
	$(CC) -c $(CFLAGS) $< -o $@

startup.o : startup.c Makefile
	$(CC) -c $(CFLAGS) -O1 startup.c -o startup.o
		
clean :
	touch Makefile
	rm -f $(OBJS)
	rm  -f *.map
	rm  -f *.bin
	rm  -f *.axf
	rm  -f *.elf




