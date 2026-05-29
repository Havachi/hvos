#ifndef HVOS_TIME_H
#define HVOS_TIME_H

#include <stdint.h>
typedef struct {
	uint8_t century;
	uint8_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} datetime_t;

typedef unsigned long long timestamp_t;

char * datetime_to_str(datetime_t * dt);
char *get_current_datetime_str();
datetime_t *now();
timestamp_t dttots(datetime_t *dt);

void init_rtc();

#endif