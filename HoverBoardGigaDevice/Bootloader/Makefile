PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
MCU = -mcpu=cortex-m3 -mthumb
CFLAGS = $(MCU) -Os -Wall -ffunction-sections -fdata-sections -I.
LDFLAGS = $(MCU) -Tlinker.ld -Wl,--gc-sections -nostartfiles -specs=nano.specs -lc -lnosys

# Master BL (USART0 host + USART1 relay). Slave BL: make SLAVE=1
ifdef SLAVE
  CFLAGS += -DIAP_USE_USART1
  BUILD_DIR = build/slave
  TARGET = bootloader-slave
else
  BUILD_DIR = build/master
  TARGET = bootloader-master
endif

SRCS = startup.c main.c
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)

all: $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(CP) -O binary -S $< $@

clean:
	rm -rf build

.PHONY: all clean
