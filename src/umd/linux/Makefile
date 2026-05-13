COMPILER_SUFFIX = gcc
CC = $(addprefix $(TOOLCHAIN_PREFIX),$(COMPILER_SUFFIX))

#CFLAGS = -Os -ggdb -Wall

INCLUDES = -I./ -I../common/inc/ -I../lib

SRCS_C = strela_test_main.c strela_bypass_test.c ../common/utilities.c ../lib/dmabuf_util.c ../lib/accel_lib.c

OBJS_C = $(SRCS_C:.c=.o)

MAIN = strela_test_linux.elf

.PHONY: clean

$(MAIN): $(OBJS_C)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(OBJS_C) -o $(MAIN)
	@echo "LD    >= $(MAIN)"

%.o: %.c
	@$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@
	@echo "CC    <= $<"

%.o: %.S
	@$(CC) $(CFLAGS) $(CCASFLAGS) $(INCLUDES) -c $<  -o $@
	@echo "CC    <= $<"

clean:
	$(RM) $(OBJS_C) $(OBJS_S) $(MAIN)
