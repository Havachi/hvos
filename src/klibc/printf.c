#include "klibc/printf.h"
#include "klibc/string.h"
#include "kernel/video.h"
#include "kernel/sync.h"
static safe_lock_t print_lock = {0};

void kprintf(const char * format, ...) {
	va_list args;
	va_start(args, format);
	kvprintf(format, args);
	va_end(args);
}

void kvprintf(const char *format, va_list args) {
	uint64_t flags = safe_lock(&print_lock);
	for(const char *p = format; *p != '\0'; p++) {
		if (*p != '%') {
			put_char(*p);
			continue;
		}
		p++;
		int ljust = 0;
		int width = 0;
		int fmtlen = 0;
		char padding = ' ';
		if (*p == '-') {
			ljust = 1;
			p++;
		}

		if (*p == '0') {
			padding = '0';
			p++;
		}


			
		while(*p >= '0' && *p <= '9') {
			width = width * 10 + (*p - '0');
			p++;
		}

		if (*p == 'l') {
			fmtlen = 1;
			p++;
		}
		switch (*p)
		{
			case 'c':{
				char c = (char)va_arg(args, int);
				for (int i = 0; i < width - 1; i++) put_char(padding);
				put_char(c);
				break;
			}

			case 's':{
				const char *s = va_arg(args, const char *);
				if (!s) s = "(null)";
				int len = kstrlen(s);
				if (ljust) {
					print(s);
					for (int i = 0; i < width - len; i++) put_char(padding);
				} else {
					for (int i = 0; i < width - len; i++) put_char(padding);
					print(s);
				}

				break;
			}

			case 'd': {
				uint32_t i = va_arg(args, uint32_t);
				char buf[32];
				int is_neg = (i < 0);
				uint32_t val = is_neg ? -i : i;
				itoa(val, buf, 10);
				int len = kstrlen(buf) + (is_neg ? 1 : 0);
				if (is_neg)put_char('-');
				for (int j = 0; j < width - len; j++) put_char(padding);
				print(buf);
				break;

			}
			case 'x': {
                char buf[32];

				if (fmtlen == 1) {
					uint64_t i = va_arg(args, uint64_t);
                	litoa(i, buf, 16);
				} else {
					int i = va_arg(args, int);
                	itoa(i, buf, 16);
				}
                int len = kstrlen(buf) + 2;
                
                print("0x");
                for (int j = 0; j < width - (len-2); j++) put_char(padding);
                print(buf);
                break;
			}
			case 'p': {
             uintptr_t ptr = (uintptr_t)va_arg(args, void *);
                char buf[32];
                itoa(ptr, buf, 16);
                int len = kstrlen(buf) + 2;
                
                print("0x");
                for (int j = 0; j < width - len; j++) put_char(padding); // Padding
                print(buf);
                break;
			}
			case '%': {
				print("%");
				break;
			}
			default:
				put_char(*p);
				break;
		}
	}
	safe_unlock(&print_lock, flags);
}