# 默认平台
PLATFORM ?= linux

# =========================
# 可配置参数
# =========================
TARGET      ?= rest
SRC_DIR     := . $(PLATFORM)
BUILD_DIR   ?= build/$(PLATFORM)

SRCS        := $(foreach dir,$(SRC_DIR),$(wildcard $(dir)/*.c))
OBJS        := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))


# =========================
# 不同平台配置
# =========================
ifeq ($(PLATFORM), linux)
    CC      := gcc
    CFLAGS  ?= -Wall -O2 $(shell pkg-config --cflags gtk4)
    LDFLAGS := $(shell pkg-config --libs gtk4)
endif

ifeq ($(PLATFORM), arm)
    CC      := arm-linux-gnueabihf-gcc
    CFLAGS  ?= -Wall -O2
    LDFLAGS :=
endif

ifeq ($(PLATFORM), aarch64)
    CC      := aarch64-linux-gnu-gcc
    CFLAGS  ?= -Wall -O2
    LDFLAGS :=
endif

ifeq ($(PLATFORM), windows)
    CC      := x86_64-w64-mingw32-gcc
    CFLAGS  ?= -Wall -O2
    LDFLAGS := -lgdi32 -luser32 -lwtsapi32
    TARGET  := $(TARGET).exe
endif

CFLAGS  += $(EXTRA_CFLAGS)

# =========================
# 构建规则
# =========================
all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $^ -o $(BUILD_DIR)/$@ $(LDFLAGS)

%.o: %.c
	@mkdir -p $(BUILD_DIR)/$(dir $@)
	$(CC) $(CFLAGS) -c $< -o $(BUILD_DIR)/$@

# =========================
# 清理
# =========================
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# =========================
# 打印配置（调试用）
# =========================
info:
	@echo "Platform: $(PLATFORM)"
	@echo "Compiler: $(CC)"
