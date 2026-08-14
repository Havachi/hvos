#include "data_structure/uuid.h"
#include <stdint.h>

extern int rand32(uint32_t *rand);

void gen_uuid(uint8_t uuid[16]) {
	uint32_t *dest = (uint32_t *)uuid;

	for (int i = 0; i < 4; i++) {
		while (!rand32(&dest[i])) {
			
		}
	}

	uuid[6] = (uuid[6] & 0x0F) | 0x40;
	uuid[8] = (uuid[8] & 0x3F) | 0x80;
}