# Nuke built-in rules.
.SUFFIXES:

# This is the name that our final executable will have.
# Change as needed.
override OUTPUT := hvos

USER_OUTPUT_INIT := init.elf

# User controllable toolchain and toolchain prefix.
TOOLCHAIN :=
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

# Internal C flags that should not be changed by the user.
override CFLAGS += \
	-Wall \
	-Wextra \
	-std=gnu11 \
	-ffreestanding \
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
USERLANDLIB := $(USERLANDDIR)/hvos.c $(USERLANDDIR)/liballoc.c
USER_CFLAGS := -Wall -Wextra -std=gnu11 -ffreestanding -nostdlib -static -no-pie -fno-pie -fno-stack-protector -fno-stack-check -O2 -Wl,-T,userland/user.lds


# Default target. This must come first, before header dependencies.
.PHONY: all
all: bin/$(OUTPUT) $(USERLANDPROG)

# Include header dependencies.
-include $(HEADER_DEPS)

# Link rules for the final executable.
bin/$(OUTPUT): GNUmakefile linker.lds $(OBJ)
	mkdir -p "$(dir $@)"
	$(LD) $(LDFLAGS) $(OBJ) -o $@

# Compilation rules for *.c files.
obj/%.c.o: %.c GNUmakefile
	mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Compilation rules for *.S files.
obj/%.S.o: %.S GNUmakefile
	mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Compilation rules for *.asm (nasm) files.
obj/%.asm.o: %.asm GNUmakefile
	mkdir -p "$(dir $@)"
	nasm $(NASMFLAGS) $< -o $@

bin/%.elf: $(USERLANDDIR)/%.c $(USERLANDLIB) GNUmakefile
	mkdir -p "$(dir $@)"
	$(CC) $(USER_CFLAGS) $< $(USERLANDLIB) -o $@

$(INITRAMFSFILEPATH): $(USERLANDPROG)
	mkdir -p $(INITRAMFSDIR)
	touch $(INITRAMFSDIR)/hello.txt
	@echo "Funny text" > $(INITRAMFSDIR)/hello.txt
	cp $(USERLANDPROG) $(INITRAMFSDIR)/
	tar --format=ustar -cvf $(INITRAMFSFILEPATH) -C $(INITRAMFSDIR) .


$(ISOOUT): bin/$(OUTPUT) $(INITRAMFSFILEPATH)
	mkdir -p $(ISOROOTDIR)
	mkdir -p $(ISOROOTBOOTDIR)
	cp bin/$(OUTPUT) $(ISOROOTBOOTDIR) 
	mkdir -p $(ISOROOTBOOTDIR)/limine
	cp limine.conf $(LIMINEBIOSFILES) $(ISOROOTBOOTDIR)/limine/
	mkdir -p $(ISOROOTDIR)/EFI/BOOT
	cp $(LIMINEEFIFILES) $(ISOROOTDIR)/EFI/BOOT
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
	-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(ISOROOTDIR) -o $(ISOOUT)



installbios:
	./limine-binary/limine bios-install $(ISOOUT)

rundbg: $(ISOOUT) installbios
	qemu-system-x86_64 -machine pc,accel=tcg,smm=off -m 1G -cdrom $(ISOOUT) -monitor stdio -d int

run: $(ISOOUT) installbios
	qemu-system-x86_64 -machine pc,accel=tcg,smm=off -m 1G -cdrom $(ISOOUT) -monitor stdio -smp 4

# Remove object files and the final executable.
clean:
	rm -rf bin obj

fclean: clean
	rm -rf $(ISOOUT)	
	rm -f $(ISOROOTBOOTDIR)/$(OUTPUT)
	rm -rf $(INITRAMFSDIR)
	rm -f $(INITRAMFSFILEPATH)

re: fclean $(ISOOUT)

kernel: bin/$(OUTPUT)
userspace: $(USERLANDPROG)
hvlibc:
	make -C hvlibc


.PHONY: all mkramfs clean hvlibc userspace kernel re fclean run rundbg installbios