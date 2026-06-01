#include "asm/asm.h"
#include "cpu/io.h"
#include <stdint.h>
#include "kernel/time.h"
#include "klibc/printf.h"
#include "klibc/string.h"
#include "mem/mem.h"

datetime_t current_datetime;
char * weekday_map[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

int is_updating_rtc() {
	io_write_8(PORT_CMOS_INDEX, 0x0A);
	uint32_t status = io_read_8(PORT_CMOS_DATA);
	return (status & 0x80);
}

static uint32_t get_rtc_reg(uint32_t reg) {
	io_write_8(PORT_CMOS_INDEX, reg);
	return io_read_8(PORT_CMOS_DATA);
}

static void set_rtc_reg(uint32_t reg, uint8_t val) {
	io_write_8(PORT_CMOS_INDEX, reg);
	io_write_8(PORT_CMOS_DATA, val);
}

void rtc_read_datetime() {
	while (is_updating_rtc());
	current_datetime.second = get_rtc_reg(0x00);
	current_datetime.minute = get_rtc_reg(0x02);
	current_datetime.hour = get_rtc_reg(0x04);
	current_datetime.day = get_rtc_reg(0x07);
	current_datetime.month = get_rtc_reg(0x08);
	current_datetime.year = get_rtc_reg(0x09);
	uint8_t reg_b = get_rtc_reg(0x0B);
	if (!(reg_b & 0x04)) {
        current_datetime.second = (current_datetime.second & 0x0F) + ((current_datetime.second / 16) * 10);
        current_datetime.minute = (current_datetime.minute & 0x0F) + ((current_datetime.minute / 16) * 10);
        current_datetime.hour = ( (current_datetime.hour & 0x0F) + (((current_datetime.hour & 0x70) / 16) * 10) ) | (current_datetime.hour & 0x80);
        current_datetime.day = (current_datetime.day & 0x0F) + ((current_datetime.day / 16) * 10);
        current_datetime.month = (current_datetime.month & 0x0F) + ((current_datetime.month / 16) * 10);
        current_datetime.year = (current_datetime.year & 0x0F) + ((current_datetime.year / 16) * 10);
	}
}

int is_leap_year(int year, int month) {
    if(year % 4 == 0 && (month == 1 || month == 2)) return 1;
    return 0;
}

int get_weekday_from_date(datetime_t * dt) {
    char month_code_array[] = {0x0,0x3, 0x3, 0x6, 0x1, 0x4, 0x6, 0x2, 0x5, 0x0, 0x3, 0x5};
    char century_code_array[] = {0x4, 0x2, 0x0, 0x6, 0x4, 0x2, 0x0};    // Starting from 18 century

    // Simple fix...
    dt->century = 21;

    // Calculate year code
    int year_code = (dt->year + (dt->year / 4)) % 7;
    int month_code = month_code_array[dt->month - 1];
    int century_code = century_code_array[dt->century - 1 - 17];
    int leap_year_code = is_leap_year(dt->year, dt->month);

    int ret = (year_code + month_code + century_code + dt->day - leap_year_code) % 7;
    return ret;
}

char * datetime_to_str(datetime_t * dt) {
    char *ret = kcalloc(22, 1);
    const char * weekday = weekday_map[get_weekday_from_date(dt)];
	ksprintf(ret, "%s %02d.%02d.%02d %02d:%02d:%02d", weekday, dt->day, dt->month, dt->year, ((dt->hour + 2) % 24), dt->minute, dt->second);
    return ret;
}

void rtc_write_datetime(datetime_t * dt) {
    // Wait until rtc is not updating
    while(is_updating_rtc());

    set_rtc_reg(0x00, dt->second);
    set_rtc_reg(0x02, dt->minute);
    set_rtc_reg(0x04, dt->hour);
    set_rtc_reg(0x07, dt->day);
    set_rtc_reg(0x08, dt->month);
    set_rtc_reg(0x09, dt->year);
}

char *get_current_datetime_str() {
    return datetime_to_str(&current_datetime);
}

datetime_t *now() {
	rtc_read_datetime();
	return &current_datetime;
}

///Datetime to timestamp
timestamp_t dttots(datetime_t *dt) {
	long long full_year = (dt->century * 100) + dt->year;
	long long month = dt->month;
	long long day = dt->day;

	if (month <= 2) {
		month += 12;
		full_year -= 1;
	}
	month -= 3;

	long long days_since_zero = day
		+ (153 * month + 2) / 5
		+ 365 * full_year
		+ full_year / 4
		- full_year / 100
		+ full_year / 400;

	const long long days_to_1970 = 719468;
	long long epoch_days = days_since_zero - days_to_1970;
	timestamp_t ts = (epoch_days * 86400ULL)
		+ (dt->hour * 3600ULL)
		+ (dt->minute * 60ULL)
		+ dt->second;

	return ts;
}

/// Convert timestamp to datetime struct (une petite dinguerie en vrai)
datetime_t *tstodt(timestamp_t ts) {
	datetime_t *dt = kmalloc(sizeof(datetime_t));

	uint32_t second_per_day = 86400;
	long long epch_days = ts / second_per_day;
	uint32_t day_remainder = ts % second_per_day;

	dt->hour = day_remainder / 3600;
	day_remainder %= 3600;
	dt->minute = day_remainder / 60;
	dt->second = day_remainder % 60;

	long long z = epch_days + 719468;
	long long era = z / 146097;
	unsigned int doe = z % 146097;

	unsigned int yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long long full_year = yoe + era * 400;
    unsigned int doy = doe - (365 * yoe + yoe / 4 - yoe / 100);

	unsigned int mp = (5 * doy + 2) / 153;	
    dt->day = doy - (153 * mp + 2) / 5 + 1;
    dt->month = (mp < 10) ? (mp + 3) : (mp - 9);

	if (dt->month <= 2) {
        full_year += 1;
    }
    dt->century = full_year / 100;
    dt->year = full_year % 100;
	return dt;
}

void init_rtc() {
	rtc_read_datetime();
}