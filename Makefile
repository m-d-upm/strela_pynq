SRC_PATH := $(PWD)/src
KSRC := $(PWD)/3rdparty/linux-xlnx

.PHONY: all clean

all: kernel-build-module test

kernel-build-module:
	@cd $(SRC_PATH)/kmd; \
	$(MAKE) KDIR=$(KSRC) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) all

test:
	@cd $(SRC_PATH)/umd; \
	$(MAKE) TOOLCHAIN_PREFIX=$(CROSS_COMPILE)

kmd-clean:
	@cd $(SRC_PATH)/kmd; \
	$(MAKE) KDIR=$(KSRC) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean

umd-clean:
	@cd $(SRC_PATH)/umd; \
	$(MAKE) clean

clean: kmd-clean umd-clean
