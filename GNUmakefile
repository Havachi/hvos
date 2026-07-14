# Nuke built-in rules.
.SUFFIXES:


export HVOS_VERSION_MAJOR := 0
export HVOS_VERSION_MINOR := 3

# This is the name that our final executable will have.
# Change as needed.
override OUTPUT := hvos

USER_OUTPUT_INIT := init.elf

# User controllable toolchain and toolchain prefix.
TOOLCHAIN := x86_64-elf
TOOLCHAIN_PREFIX := 
ifneq ($(TOOLCHAIN),)
	ifeq ($(TOOLCHAIN_PREFIX),)
		TOOLCHAIN_PREFIX := $(TOOLCHAIN)-
	endif
endif

# User controllable C compiler command.
ifneq ($(TOOLCHAIN_PREFIX),)
	CC := $(TOOLCHAIN_PREFIX)gcc
else
	CC := cc
endif

# User controllable linker command.
LD := $(TOOLCHAIN_PREFIX)ld
LDU := x86_64-hvos-ld

# Defaults overrides for variables if using "llvm" as toolchain.
ifeq ($(TOOLCHAIN),llvm)
	CC := clang
	LD := ld.lld
endif

# User controllable C flags.
CFLAGS := -g -Og -pipe -I./src/include

# User controllable C preprocessor flags. We set none by default.
CPPFLAGS :=

# User controllable nasm flags.
NASMFLAGS := -g

# User controllable linker flags. We set none by default.
LDFLAGS :=

# Check if CC is Clang.
override CC_IS_CLANG := $(shell ! $(CC) --version 2>/dev/null | grep -q '^Target: '; echo $$?)

# If the C compiler is Clang, set the target as needed.
ifeq ($(CC_IS_CLANG),1)
	override CC += \
		-target x86_64-unknown-none-elf
endif


SYSROOT_DIR?=sysroot

# Internal C flags that should not be changed by the user.
override CFLAGS += \
	-Wall \
	-Wextra \
	-std=gnu11 \
	-ffreestanding \
	-nostdlib \
	-fno-stack-protector \
	-fno-stack-check \
	-fno-lto \
	-fno-PIC \
	-ffunction-sections \
	-fdata-sections \
	-m64 \
	-march=x86-64 \
	-mabi=sysv \
	-mno-80387 \
	-mno-mmx \
	-mno-sse \
	-mno-sse2 \
	-mno-red-zone \
	-mcmodel=kernel \
	-ggdb \
	--sysroot=$(SYSROOT_DIR) \
	-isystem=/usr/include \
	-DHVOS_VERSION_MAJOR=\"$(HVOS_VERSION_MAJOR)\" \
	-DHVOS_VERSION_MINOR=\"$(HVOS_VERSION_MINOR)\" \
	-O0

# Internal C preprocessor flags that should not be changed by the user.
override CPPFLAGS := \
	-I src \
	$(CPPFLAGS) \
	-MMD \
	-MP

# Internal nasm flags that should not be changed by the user.
override NASMFLAGS := \
	-f elf64 \
	$(patsubst -g,-g -F dwarf,$(NASMFLAGS)) \
	-g \
	-Wall

# Internal linker flags that should not be changed by the user.
override LDFLAGS += \
	-m elf_x86_64 \
	-nostdlib \
	-static \
	-z max-page-size=0x1000 \
	--gc-sections \
	--sysroot=$(SYSROOT_DIR) \
	-L$(SYSROOT_DIR)/usr/lib\
	-T linker.lds

# Use "find" to glob all *.c, *.S, and *.asm files in the tree and obtain the
# object and header dependency file names.
override SRCFILES := $(shell find -L src -type f 2>/dev/null | LC_ALL=C sort)
override CFILES := $(filter %.c,$(SRCFILES))
override ASFILES := $(filter %.S,$(SRCFILES))
override NASMFILES := $(filter %.asm,$(SRCFILES))
override OBJ := $(addprefix obj/,$(CFILES:.c=.c.o) $(ASFILES:.S=.S.o) $(NASMFILES:.asm=.asm.o))
override HEADER_DEPS := $(addprefix obj/,$(CFILES:.c=.c.d) $(ASFILES:.S=.S.d))
ISOROOTDIR := iso_root
ISOROOTBOOTDIR := $(ISOROOTDIR)/boot
LIMINEBIOSFILES := limine-binary/limine-bios.sys limine-binary/limine-bios-cd.bin limine-binary/limine-uefi-cd.bin
LIMINEEFIFILES := limine-binary/BOOTX64.EFI limine-binary/BOOTIA32.EFI
ISOOUT :=$(OUTPUT)-boot.iso
INITRAMFSFILEPATH := $(ISOROOTDIR)/initramfs.tar
INITRAMFSDIR := initramfs
USERLANDDIR := userland
USERLANDSRC := $(USERLANDDIR)/init.c $(USERLANDDIR)/shell.c
USERLANDPROG := bin/init.elf bin/shell.elf
USERLANDLIB := $(USERLANDDIR)/hvos.c

USER_CFLAGS := -Wall -Wextra -std=gnu11 -ffreestanding \
	 -static -no-pie -fno-pie -fno-stack-protector -fno-stack-check \
	  -O2 -Wl,-T,userland/user.lds \
	  --sysroot=$(SYSROOT_DIR) -isystem$(SYSROOT_DIR)/usr/include 
HVLIBC_DIR := hvlibc

INSTALLED_LIBK:= $(SYSROOT_DIR)/usr/lib/libk.a
INSTALLED_LIBC:= $(SYSROOT_DIR)/usr/lib/libc.a

DRIVE_FILE := drv0.img

# Default target. This must come first, before header dependencies.
.PHONY: all
all: bin/$(OUTPUT) $(USERLANDPROG)

# Include header dependencies.
-include $(HEADER_DEPS)

# Link rules for the final executable.
bin/$(OUTPUT): GNUmakefile linker.lds $(INSTALLED_LIBK) $(OBJ)
	@mkdir -p "$(dir $@)"
	@echo "[LD] $(notdir $@)"
	@$(LD) $(LDFLAGS) $(OBJ) -lk -o $@

# Compilation rules for *.c files.
obj/%.c.o: %.c GNUmakefile
	@mkdir -p "$(dir $@)"
	@echo "[CC] $(notdir $@)"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Compilation rules for *.S files.
obj/%.S.o: %.S GNUmakefile
	@mkdir -p "$(dir $@)"
	@echo "[AS] $(notdir $@)"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Compilation rules for *.asm (nasm) files.
obj/%.asm.o: %.asm GNUmakefile
	@mkdir -p "$(dir $@)"
	@echo "[NASM] $(notdir $@)"
	@nasm $(NASMFLAGS) $< -o $@

$(INITRAMFSFILEPATH): $(USERLANDPROG)
	@mkdir -p $(INITRAMFSDIR)
	@touch $(INITRAMFSDIR)/hello.txt
	@echo "Funny text" > $(INITRAMFSDIR)/hello.txt
	@cp $(USERLANDPROG) $(INITRAMFSDIR)/
	@tar --format=ustar -cvf $(INITRAMFSFILEPATH) -C $(INITRAMFSDIR) .


$(ISOOUT): bin/$(OUTPUT) $(INITRAMFSFILEPATH)
	@mkdir -p $(ISOROOTDIR)
	@mkdir -p $(ISOROOTBOOTDIR)
	@cp bin/$(OUTPUT) $(ISOROOTBOOTDIR) 
	@mkdir -p $(ISOROOTBOOTDIR)/limine
	@cp limine.conf $(LIMINEBIOSFILES) $(ISOROOTBOOTDIR)/limine/
	@mkdir -p $(ISOROOTDIR)/EFI/BOOT
	@cp $(LIMINEEFIFILES) $(ISOROOTDIR)/EFI/BOOT
	@xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
	-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(ISOROOTDIR) -o $(ISOOUT)


QEMU_DRIVE_FLAGS := -device ich9-ahci,id=ahci \
					-drive id=boot_cd,file=$(ISOOUT),format=raw,if=none \
					-device ide-cd,drive=boot_cd,bus=ahci.0 \
					-drive id=disk0,file=$(DRIVE_FILE),format=raw,if=none \
					-device ide-hd,drive=disk0,bus=ahci.1 \
					-boot d

installbios:
	@./limine-binary/limine bios-install $(ISOOUT)

rundbg: $(ISOOUT) installbios $(DRIVE_FILE)
	@qemu-system-x86_64 -S -gdb tcp::1234 -m 1G -smp 4 -daemonize -d int $(QEMU_DRIVE_FLAGS)
runint: $(ISOOUT) installbios $(DRIVE_FILE)
	@qemu-system-x86_64 -machine pc,accel=tcg,smm=off -m 1G -monitor stdio -d int -no-reboot -no-shutdown $(QEMU_DRIVE_FLAGS)

run: $(ISOOUT) installbios $(DRIVE_FILE)
	@qemu-system-x86_64 -machine pc,accel=tcg,smm=off -m 2G -monitor stdio -smp 4 $(QEMU_DRIVE_FLAGS)



# Remove object files and the final executable.
clean:
	@rm -rf bin obj

fclean: clean
	@rm -rf $(ISOOUT)	
	@rm -f $(ISOROOTBOOTDIR)/$(OUTPUT)
	@rm -rf $(INITRAMFSDIR)
	@rm -f $(INITRAMFSFILEPATH)

re: fclean $(ISOOUT)

kernel: bin/$(OUTPUT)
userspace:
	@make -C $(USERLANDDIR)

$(USERLANDPROG): userspace $(INSTALLED_LIBC)
	@cp $(USERLANDDIR)/build/*.elf ./bin


$(INSTALLED_LIBC): mkfullsysroot
	@SYSROOT_DIR=../$(SYSROOT_DIR) make --no-print-directory -C hvlibc install_libc

$(INSTALLED_LIBK): mkfullsysroot
	@SYSROOT_DIR=../$(SYSROOT_DIR) make --no-print-directory -C hvlibc install_libk

fclean_lib:
	@make --no-print-directory -C hvlibc fclean

$(SYSROOT_DIR):
	@mkdir -p $(SYSROOT_DIR)

mkbasesysroot: $(SYSROOT_DIR)
	@mkdir -p $(SYSROOT_DIR)/bin $(SYSROOT_DIR)/lib
	@mkdir -p $(SYSROOT_DIR)/usr/bin $(SYSROOT_DIR)/usr/lib

mkfullsysroot: $(SYSROOT_DIR)
	@mkdir -p \
		$(SYSROOT_DIR)/bin \
		$(SYSROOT_DIR)/etc \
		$(SYSROOT_DIR)/sbin \
		$(SYSROOT_DIR)/usr \
		$(SYSROOT_DIR)/usr/bin \
		$(SYSROOT_DIR)/usr/include \
		$(SYSROOT_DIR)/usr/lib \
		$(SYSROOT_DIR)/usr/local \
		$(SYSROOT_DIR)/usr/share \

install_headers: install_hvlibc_header install_hvos_header

install_hvlibc: $(INSTALLED_LIBC)



install_hvlibc_header:
	@SYSROOT_DIR=../$(SYSROOT_DIR) make --no-print-directory -C hvlibc install_headers

install_hvos_header:
	@cp -RT src/include $(SYSROOT_DIR)/usr/include

fclean_sysroot:
	rm -rf $(SYSROOT_DIR)

$(DRIVE_FILE):
	qemu-img create -f raw $(DRIVE_FILE) 256M

full: fclean fclean_lib fclean_sysroot mkbasesysroot install_hvlibc install_headers all

.PHONY: all mkramfs clean hvlibc userspace kernel re fclean run rundbg installbios