TARGET_NAME = synstack

SRC_DIR = src
INC_DIR = src
EX_DIR = examples
BUILD_ROOT = build
LIB_ROOT = lib
EX_BIN_DIR = bin

DEFINES =
WARN = -Wall
CFLAGS = -std=c11 -I$(INC_DIR)
LDFLAGS = -l$(TARGET_NAME) -lncurses -lportaudio

ifndef verbose
SILENT = @
endif

ifndef config
config=debug
endif

ifndef platform
platform = native
endif

_SOURCES = \
	4pole \
	adsr \
	biquad \
	delay \
	foldback \
	formant \
	iir \
	node_ops \
	notes \
	osc \
	panning \
	pluck \
	synth \
	wavfile

_EX_SOURCES = \
	synth_drone \
	synth_keys \
	synth_pan \
	synth_render \
	synth_formantseq \
	synth_ksensemble \
	synth_pwm \
	synth_spiral

LIB_DIR = $(LIB_ROOT)/$(config)/$(platform)
BUILD_DIR = $(BUILD_ROOT)/$(config)/$(platform)
TARGET = $(LIB_DIR)/lib$(TARGET_NAME).a

SOURCES = $(addsuffix .c, $(addprefix $(SRC_DIR)/, $(_SOURCES)))
DEPS = $(addsuffix .h, $(addprefix $(SRC_DIR)/, $(_SOURCES)))
EX_SOURCES = $(addsuffix .c, $(addprefix $(EX_DIR)/, $(_EX_SOURCES)))
OBJS = $(addsuffix .o, $(addprefix $(BUILD_DIR)/, $(_SOURCES)))
EX_OBJS = $(addprefix $(EX_BIN_DIR)/, $(_EX_SOURCES))

ifneq (,$(findstring stm32, $(platform)))
CC_PREFIX = arm-none-eabi
CC = $(CC_PREFIX)-gcc
AR = $(CC_PREFIX)-ar
OBJCOPY = $(CC_PREFIX)-objcopy
LDFLAGS += -nostartfiles
endif

ifeq (stm32f4, $(platform))
DEFINES += -DSTM32F401xC
CFLAGS += -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
LDFLAGS += -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
else ifeq (stm32f7, $(platform))
DEFINES += -DSTM32F746xx
CFLAGS += -mthumb -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-sp-d16 -Wa,-adhlns="$@.lst"
LDFLAGS += -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
endif

ifeq (debug, $(config))
DEFINES += -DDEBUG
CFLAGS += -g3
else ifeq (release, $(config))
DEFINES += -DNDEBUG
CFLAGS += -Os
endif

CFLAGS += $(WARN) $(DEFINES)
LIB_CMD = $(AR) -rcs "$@" $(OBJS)

all: lib examples

lib: prologue $(BUILD_DIR) $(LIB_DIR) $(TARGET) epilogue

examples: lib $(EX_BIN_DIR) $(EX_OBJS)

prologue:
	@echo "==== Building target: $(TARGET) ===="
	@echo "==== Config: $(config)"
	@echo "==== Flags: $(CFLAGS)"
	@echo "==== Obj: $(OBJS)"

epilogue:
	@echo "done."

clean:
	@echo "==== Cleaning all"
	$(SILENT) rm -rf $(BUILD_ROOT)
	$(SILENT) rm -rf $(LIB_ROOT)
	$(SILENT) rm -rf $(EX_BIN_DIR)

$(TARGET): $(OBJS)
	@echo "==== Bundling library"
	$(SILENT) $(LIB_CMD)

$(LIB_DIR):
	@echo "==== Creating $(LIB_DIR)"
	$(SILENT) mkdir -p $(LIB_DIR)

$(BUILD_DIR):
	@echo "==== Creating $(BUILD_DIR)"
	$(SILENT) mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/%.h
	@echo "==== Compiling: $(notdir $<)"
	$(SILENT) $(CC) $(CFLAGS) -c $< -o $@

### Examples

$(EX_BIN_DIR):
	@echo "==== Creating $(EX_BIN_DIR)"
	$(SILENT) mkdir -p $(EX_BIN_DIR)

$(EX_BIN_DIR)/%: $(EX_SOURCES) $(EX_DIR)/demo_common.c $(EX_DIR)/demo_common.h
	@echo "=== Compiling example: $(notdir $@)"
	$(CC) $(CFLAGS) -I$(EX_DIR) -L$(LIB_DIR) $(LDFLAGS) -o $@ $(EX_DIR)/$(notdir $@).c $(EX_DIR)/demo_common.c

.PHONY: all lib examples clean prologue epilogue
