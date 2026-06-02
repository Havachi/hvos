#ifndef HVOS_TAR_H
#define HVOS_TAR_H


#define TAR_TYPE_NORMAL '0'
#define TAR_TYPE_DIR '5'

typedef struct {
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char typeflag;
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
	char padding[12];
} __attribute__((packed)) tar_header_t;


#endif