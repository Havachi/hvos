# HVOS

HVOS is just a little x86_64 hobby-os that i've made.

Currently the it's in a good shape. Not a lot of functionality but at least it's not crashing for no reason (or is it...). 

This makes me happy.

## To implement

- [x] Userspace dynamic memory allocation (malloc)
	- [x] New syscall
		- [x] mmap/munmap
	- [x] Implement liballoc function in userspace

- [x] Find a solution for the growing userspace libs
	- [x] Make hvlibc
- [ ] Filesystem (a big piece)
	- [ ] Make current VFS more complete
	- [ ] Disk driver
	- [ ] Fat32
	- [ ] ext4 ???

- [ ] Make shell better
	- [ ] New commands !
	- [ ] Colors !

- [ ] Git
  - [ ] Use submodule for limine-binaries and build the on the fly


## Prerequisites
- make
- clang
- qemu


## Running


Running in qemu:
```
make
make run
```
