#ifndef _HVOS_FILE_H
#define _HVOS_FILE_H

struct file;

struct fd {
	unsigned long word;
};


struct fd fdget(unsigned int fd);

#endif