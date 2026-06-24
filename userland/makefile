CC := x86_64-elf-gcc
LD := x86_64-hvos-ld
AR := x86_64-hvos-ar

HVOS_VERSION_MAJOR ?= 0
HVOS_VERSION_MINOR ?= 1

SYSROOT ?= ../sysroot
LIBDIR := $(SYSROOT)/usr/lib
INCDIR := $(SYSROOT)/usr/include

CFLAGS := --sysroot=$(SYSROOT) -isystem $(INCDIR) -D__hvos__ -g -Og -Wall -Wextra -DHVOS_VERSION_MAJOR=$(HVOS_VERSION_MAJOR) \
	-DHVOS_VERSION_MINOR=$(HVOS_VERSION_MINOR)
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
HVOSLIB_INSTALLED := $(LIBDIR)/libhvos.a

.PHONY: all clean

all: $(BUILD_DIR) $(HVOSLIB_INSTALLED) $(BINS)

$(LIBOBJ): $(LIBSRC)
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -c $< -o $*.o -lc

$(HVOSLIB): $(LIBOBJ)
	@echo "[AR] $<"
	@$(AR) rcs $@ $(LIBOBJ)

$(HVOSLIB_INSTALLED): $(HVOSLIB)
	@cp $(HVOSLIB) $(LIBDIR)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	
$(BUILD_DIR)/%: %.c $(HVOSLIB_INSTALLED) 
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -c $< -o $(BUILD_DIR)/$*.o 
	@echo "[LD] $@"
	@$(LD) $(LDFLAGS) -T./user.lds -o $@.elf \
		$(CRT0) \
		$(CRTI) \
		$(BUILD_DIR)/$*.o \
		--start-group -lc -lhvos --end-group \
		$(CRTN)

clean:
	rm -rf $(BUILD_DIR)