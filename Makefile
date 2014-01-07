#############################################################################################################
#
#------------------------ Project : Horrorophone
# ----------------------- Makefile for STM32F4 Discovery board projects ------------------------------
#
#
#################################
NAME = horrorophone
#################################


TARGET=$(NAME).hex
EXECUTABLE=$(NAME).elf

SRC = src/main.c \
	src/stm32f4xx_it.c \
	src/system_stm32f4xx.c \
	src/saw_osc.c \
	src/minblep_tables.c \
	src/stm32f4_discovery_audio_codec.c \
	src/stm32f4_discovery.c \
	lib/src/peripherals/stm32f4xx_syscfg.c \
	lib/src/peripherals/misc.c \
	lib/src/peripherals/stm32f4xx_adc.c \
	lib/src/peripherals/stm32f4xx_dma.c \
	lib/src/peripherals/stm32f4xx_exti.c \
	lib/src/peripherals/stm32f4xx_gpio.c \
	lib/src/peripherals/stm32f4xx_i2c.c \
	lib/src/peripherals/stm32f4xx_rcc.c \
	lib/src/peripherals/stm32f4xx_spi.c \
	lib/src/peripherals/stm32f4xx_tim.c \
	lib/src/peripherals/stm32f4xx_dac.c \
	lib/src/peripherals/stm32f4xx_rng.c \

STARTUP = lib/startup_stm32f4xx.s

STM32_INCLUDES = -Iinc \
	-Ilib/inc \
	-Ilib/inc/core \
	-Ilib/inc/peripherals

LINKFILE = link.ld

################################################################################################	

OPTIMIZE       = -O3


LIBDIR = 
#C:\GNUToolsARMEmbedded\4.8-2013q4\arm-none-eabi\lib\armv7e-m\fpu

CC=arm-none-eabi-gcc
#LD=arm-none-eabi-ld 
LD=arm-none-eabi-gcc
AR=arm-none-eabi-ar
AS=arm-none-eabi-as
CP=arm-none-eabi-objcopy
OD=arm-none-eabi-objdump

BIN=$(CP) -O ihex

DEFS = -D__arm__ -D__ASSEMBLY__ -DUSE_STDPERIPH_DRIVER -DSTM32F4XX -DHSE_VALUE=8000000 -D__FPU_PRESENT=1 -D__FPU_USED=1 -DSTM32F407VG

MCFLAGS = -mcpu=cortex-m4 -mthumb -mlittle-endian -mfpu=fpv4-sp-d16 -mfloat-abi=hard

CFLAGS	= $(MCFLAGS)  $(OPTIMIZE)  $(DEFS) -ffunction-sections -I./ -I./ $(STM32_INCLUDES)  -Wl,-T,$(LINKFILE)
AFLAGS	= $(MCFLAGS)

OBJDIR = .
OBJ = $(SRC:%.c=$(OBJDIR)/%.o) 
OBJ += Startup.o

################################################################################################################
all: $(TARGET) size

$(TARGET): $(EXECUTABLE)
	@echo 'Invoking: Cross ARM GNU Create Flash Image'
	$(CP) -O ihex $^ $@
	@echo 'Finished building: $@'
	@echo ' '

$(EXECUTABLE): $(SRC) $(STARTUP)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross ARM C Linker'
	$(CC) $(CFLAGS) -nostartfiles  -Wl,-Map=horrorophone.map -Wl,--gc-sections $^ -o $@ -L$(LIBDIR) -lgcc -lc -lm
	@echo 'Finished building target: $@'
	@echo ' '
	
size: $(EXECUTABLE)
	@echo 'Invoking: Cross ARM GNU Print Size'
	arm-none-eabi-size  --format=berkeley --totals $^
	@echo 'Finished building: $@'
	@echo ' '
	
clean:
	rm -f Startup.lst  $(TARGET)  $(EXECUTABLE) *.lst $(OBJ) $(AUTOGEN)  *.out *.map *.dmp


# *** EOF *****************************************************************************************************