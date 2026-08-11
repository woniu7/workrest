# 默认平台
PLATFORM ?= linux

# =========================
# 可配置参数
# =========================
TARGET      ?= rest
SRC_DIR     := . $(PLATFORM)
BUILD_DIR   ?= build/$(PLATFORM)

# 视图实现(编译期二选一)：terminal(默认) 或 gui
VIEW        ?= terminal
ifeq ($(VIEW), gui)
    VIEW_EXCLUDE := $(PLATFORM)/terminal.c
else ifeq ($(VIEW), terminal)
    VIEW_EXCLUDE := $(PLATFORM)/ui.c
else
    $(error VIEW 必须是 terminal 或 gui)
endif

SRCS        := $(foreach dir,$(SRC_DIR),$(wildcard $(dir)/*.c))
# 去掉 ./ 前缀
SRCS        := $(patsubst ./%,%,$(SRCS))
# 只保留选中的视图实现
SRCS        := $(filter-out $(VIEW_EXCLUDE),$(SRCS))

OBJS        := $(patsubst %.c,%.o,$(SRCS))


# =========================
# 不同平台配置
# =========================
ifeq ($(PLATFORM), linux)
    CC      := gcc
    ifeq ($(VIEW), gui)
        # GUI 视图需要 GTK4(其已包含 GLib)
        CFLAGS  ?= -Wall -O2 $(shell pkg-config --cflags gtk4)
        LDFLAGS := $(shell pkg-config --libs gtk4)
    else
        # 终端视图只需 GLib(主循环 + g_unix_fd_add)
        CFLAGS  ?= -Wall -O2 $(shell pkg-config --cflags glib-2.0)
        LDFLAGS := $(shell pkg-config --libs glib-2.0)
    endif
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
	$(CC) $(addprefix $(BUILD_DIR)/, $^) -o $(BUILD_DIR)/$@ $(LDFLAGS)

%.o: %.c
	@mkdir -p $(BUILD_DIR)/$(dir $@)
	$(CC) $(CFLAGS) -c $< -o $(BUILD_DIR)/$@

# =========================
# 清理
# =========================
clean:
	rm -rf $(BUILD_DIR) 

# =========================
# 打印配置（调试用）
# =========================
info:
	@echo "Platform: $(PLATFORM)"
	@echo "View:     $(VIEW)"
	@echo "Compiler: $(CC)"
	@echo "Sources:  $(SRCS)"
