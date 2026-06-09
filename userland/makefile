CC := x86_64-elf-gcc
LD := x86_64-hvos-ld
AR := x86_64-hvos-ar


SYSROOT ?= ../sysroot
LIBDIR := $(SYSROOT)/usr/lib
INCDIR := $(SYSROOT)/usr/include

CFLAGS := --sysroot=$(SYSROOT) -isystem $(INCDIR) -D__hvos__ -g -o2 -Wall -Wextra
LDFLAGS := -m elf_x86_64 --sysroot=$(SYSROOT) -L$(LIBDIR)

CRT0    := $(LIBDIR)/crt0.o
CRTI    := $(LIBDIR)/crti.o
CRTN    := $(LIBDIR)/crtn.o

BUILD_DIR := build

#SRCS := $(wildcard *.c)
SRCS := shell.c init.c
BINS := $(patsubst %.c,$(BUILD_DIR)/%,$(SRCS))

LIBSRC := hvos.c
LIBOBJ := $(BUILD_DIR)/hvos.o
HVOSLIB := $(BUILD_DIR)/libhvos.a

.PHONY: all clean

all: $(BUILD_DIR) $(HVOSLIB) $(BINS)

$(LIBOBJ): $(LIBSRC)
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -c $< -o $*.o

$(HVOSLIB): $(LIBOBJ)
	@echo "[AR] $<"
	@$(AR) rcs $@ $(LIBOBJ)
	@cp $(HVOSLIB) $(LIBDIR)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	
$(BUILD_DIR)/%: %.c
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -c $< -o $(BUILD_DIR)/$*.o
	@echo "[LD] $@"
	@$(LD) $(LDFLAGS) -o $@.elf $(CRT0) $(CRTI) $(BUILD_DIR)/$*.o -lc -lhvos $(CRTN)

clean:
	rm -rf $(BUILD_DIR)