PLATFORM ?= linux
BUILD_DIR ?= $(CURDIR)/build/$(PLATFORM)

-SRC := main.c
+SRC := rest.c
 OBJ := $(patsubst %.c,%.o,$(SRC))
-TARGET = rest
+TARGET = rest.o

 all: $(TARGET)

-# 链接
-$(TARGET): $(OBJ)
-       @mkdir -p $(dir $@)
-       $(CC) $(GTK_LFLAGS) $(addprefix $(BUILD_DIR)/, $^) -o $(BUILD_DIR)/$@
-
-# 编译
 %.o: %.c
-       @mkdir -p $(dir $@)
+       @mkdir -p $(BUILD_DIR)
        $(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $(BUILD_DIR)/$@

all: #just for default target
	$(MAKE) -C $(PLATFORM) BUILD_DIR=$(BUILD_DIR) $@

%: #not working for default without target
	$(MAKE) -C $(PLATFORM) BUILD_DIR=$(BUILD_DIR) $@
%.o: #match *.o while the % does not match
	$(MAKE) -C $(PLATFORM) BUILD_DIR=$(BUILD_DIR) $@

.PHONY: all %
