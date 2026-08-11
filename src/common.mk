# =========================
# 公共构建逻辑
# 由 src/<platform>/Makefile include。
# 调用方(平台 Makefile)需先定义:CC / CFLAGS / LDFLAGS,可选 TARGET / VIEW。
# =========================

TARGET     ?= rest
VIEW       ?= gui
BUILD_DIR  ?= build

# 公共源码所在目录(即 src/)
COMMON_DIR := ..

# 视图实现(编译期二选一):terminal(默认) 或 gui
ifeq ($(VIEW), gui)
    VIEW_EXCLUDE := terminal.c
else ifeq ($(VIEW), terminal)
    VIEW_EXCLUDE := ui.c
else
    $(error VIEW 必须是 terminal 或 gui)
endif

# 源文件:公共目录(../*.c) + 当前平台目录(./*.c),再剔除未选中的视图实现
COMMON_SRCS := $(notdir $(wildcard $(COMMON_DIR)/*.c))
PLAT_SRCS   := $(filter-out $(VIEW_EXCLUDE),$(wildcard *.c))
SRCS        := $(COMMON_SRCS) $(PLAT_SRCS)
OBJS        := $(addprefix $(BUILD_DIR)/,$(SRCS:.c=.o))

# 让 %.c 规则能同时在平台目录和公共目录里找到源文件
vpath %.c . $(COMMON_DIR)

# =========================
# 构建规则
# =========================
all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

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

.PHONY: all clean info
