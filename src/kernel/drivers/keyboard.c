#include "drivers/keyboard.h"
#include "drivers/console.h"
#include "drivers/scancode.h"
#include <stdio.h>
#include "cpu/io.h"

uint32_t kdb_queue_occupancy = 0;
static char kdb_queue[KDB_BUFFER_SIZE];
uint32_t kdb_head = 0;
uint32_t kdb_tail = 0;


extern uintptr_t lapic_virt_base;
bool key_pressed[128];
bool lshift_pressed;
bool rshift_pressed;

static uint8_t read_scancode() {
	return io_read_8(KEYBOARD_DATA_PORT);
}

extern void notify_wait_channel(void *channel);
void append_to_input_buffer(char c) {
	if(kdb_queue_occupancy < KDB_BUFFER_SIZE){
		kdb_queue[kdb_head] = c;	
		kdb_head = (kdb_head + 1) % KDB_BUFFER_SIZE;
		kdb_queue_occupancy++;
	}
	notify_wait_channel(console_read);
}




void keyboard_handler_c(void) {
		uint8_t scancode = read_scancode();
		bool pressed = 1;
		if (scancode >= 128) {
			pressed = 0;
			scancode -= 128;
		}
		key_pressed[scancode] = pressed;

		if (!pressed) {
			return;
		}
/*
		if (scancode == BACKSPACE) {
			if (kdb_queue_occupancy > 0) {
				kdb_queue_occupancy -=1;
				kdb_queue[kdb_queue_occupancy] = 0;
			}
			printf("\b");
			return;
		}
*/

		if (scancode == BACKSPACE) {
			append_to_input_buffer('\b');
			return;
		}
		char c;

		if (key_pressed[LCTRL]) {
			c = normal_scan_code_table[scancode];
			if (c == 'c') {
				append_to_input_buffer(CANCEL);

				return;

			}
			if (c == 'l') {
				append_to_input_buffer(CLEAR);
				return;
			}
		}

		if (key_pressed[LSHIFT] || key_pressed[RSHIFT]) {
			c = shift_scan_code_table[scancode];
		} else {
			c = normal_scan_code_table[scancode];
		}
		append_to_input_buffer(c);
}

char keyboard_get_char(void) {
	char c = 0;
	if (kdb_queue_occupancy > 0){
		c = kdb_queue[kdb_tail];
		kdb_tail = (kdb_tail + 1) % KDB_BUFFER_SIZE;
		kdb_queue_occupancy--;
	}
	return c;
}
