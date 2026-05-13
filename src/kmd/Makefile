SUBDIRS = $(wildcard */)

.PHONY: all modules_install clean

all:
	@for dir in $(SUBDIRS); do \
		$(MAKE) KERNEL_SRC=$(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $$dir all; \
	done

modules_install:
	for dir in $(CURRENT_DIR); do \
		$(MAKE) KERNEL_SRC=$(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $$dir modules_install; \
	done

clean:
	for dir in $(CURRENT_DIR); do \
		$(MAKE) KERNEL_SRC=$(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $$dir clean; \
	done
