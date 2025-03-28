# This Makefile can be used to manually build the kernel module.

# Get kernel version
KVER := $(shell uname -r)

# Include variables
include _variables.sh

KDIR ?= /lib/modules/$(KVER)/build
PWD := $(shell pwd)
BUILD_DIR := $(PWD)/$(BUILD_DIR)

obj-m := $(MODULE_NAME).o

all: prepare
	$(MAKE) -C $(KDIR) M=$(BUILD_DIR) src=$(PWD)/$(SRC_DIR) modules

prepare:
	@mkdir -p $(BUILD_DIR)
	@cp $(SRC_DIR)/$(MODULE_NAME).c $(SRC_DIR)/$(MODULE_NAME).h $(BUILD_DIR)/

clean:
	@if [ -d "$(BUILD_DIR)" ]; then \
		$(MAKE) -C $(KDIR) M=$(BUILD_DIR) clean; \
	fi
	@rm -rf $(BUILD_DIR)

install: all
	sudo mkdir -p /lib/modules/$(KVER)/kernel/drivers/acpi
	sudo cp $(BUILD_DIR)/$(MODULE_NAME).ko /lib/modules/$(KVER)/kernel/drivers/acpi/
	sudo depmod -a
	sudo modprobe $(MODULE_NAME)

uninstall:
	sudo rm -f /lib/modules/$(KVER)/kernel/drivers/acpi/$(MODULE_NAME).ko
	sudo depmod -a
	sudo modprobe -r $(MODULE_NAME)

# Development helpers
.PHONY: checkpatch
checkpatch:
	@$(SRC_DIR)/scripts/checkpatch.pl --no-tree -f $(SRC_DIR)/*.c $(SRC_DIR)/*.h

.PHONY: sparse
sparse:
	@make -C $(MODULES_DIR)/build M=$(BUILD_DIR) src=$(PWD)/$(SRC_DIR) C=1 CF="-D__CHECK_ENDIAN__" modules

.PHONY: coccicheck
coccicheck:
	@make -C $(MODULES_DIR)/build M=$(BUILD_DIR) src=$(PWD)/$(SRC_DIR) coccicheck
