#include "hvos.h"
#include "liballoc.h"

static char input_buffer[128];
static char history_buffer[100][128];
static int index = 0;
static int history_index = 0;
static char *olds;
static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static char *strcpy(const char* s1, char* s2) {
    while (*s1) {
		*s2++ = *s1++;
	}
	return s2;
}

static char *strtok(char *s, const char *delim) {
	char* token;
}

char prompt[7] = "hvos> ";

void clear_buffer() {
	while (index > 0) {
		input_buffer[index--] = '\0';
	}
	input_buffer[0] = '\0';
	index = 0;
}

void ls() {

}
cmd_t *parse_cmd(const char *input) {
	cmd_t *cmd = malloc(sizeof(cmd_t));
	cmd->tokens = calloc(sizeof(char) * 32, 3);
	cmd->nb_tokens = 0;

	char *tok;
	tok = strtok(input, " ");
	while (tok != NULL){
		tok = strtok(NULL, " ");
		cmd->tokens[cmd->nb_tokens++] = tok;
	}
	return cmd;
}

int exec_cmd(const char *cmd) {
	if (strcmp(input_buffer, "help") == 0) {
		user_print("Help menu !\n");
	} else if (strcmp(input_buffer, "open")) {
		open();
	} else if(strcmp(input_buffer, "exit") == 0) {
		exit(0);
	} else {
		user_printerr("hvsh: command not found: ");
		user_printerr(input_buffer);
		user_printerr("\n");
	}
}

void _start(void) {
	user_print("\nWelcome to HVOS 0.1\n");
	while (1) {
		user_print(prompt);
		index = 0;
		while (1) {
			char c;
			read(0, &c, 1);
			if (c == '\n') {
				user_print("\n");
				strcpy(history_buffer[history_index++], input_buffer); 
				break;
			} else if (c == '\b') {
				if (index > 0) {
					index--;
					input_buffer[index] = '\0';
					user_print("\b");
				}
			} else if (c == CLEAR) {
				clear_screen();
				clear_buffer();
				break;
			} else if (c == CANCEL) {
				clear_buffer();
				user_print("\n");
				break;
			} else if(index < 127) {
				input_buffer[index++] = c;
				write(1, &c, 1);
			}
		}
		if (index == 0) continue;
		exec_cmd(input_buffer);
		clear_buffer();
	}

	exit(7);
}