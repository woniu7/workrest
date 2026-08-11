# =========================
# 顶层入口:只做默认平台设置与转发。
# 实际构建逻辑在 common.mk,平台配置在 <platform>/Makefile。
#
#   make [PLATFORM=linux]        # 等价于 cd linux && make
#   make PLATFORM=windows        # 等价于 cd windows && make
#   make PLATFORM=linux VIEW=gui # 变量会透传给子 make
#   make clean / make info       # 同样转发到对应平台
# =========================

# 默认平台
PLATFORM ?= linux

# 目标统一转发到 src/<platform>/ 下构建
all clean info:
	$(MAKE) -C $(PLATFORM) $@

.PHONY: all clean info
