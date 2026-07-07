#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "hvos.h"

static char input_buffer[128];
static char history_buffer[100][128];
static int index = 0;
static int history_index = 0;
static char *olds;

char prompt[8] = "hvos> \0";

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
		printf("Help menu !\n");
	} else if (strcmp(input_buffer, "open") == 0) {
		//open();
	} else if(strcmp(input_buffer, "exit") == 0) {
		exit(0);
	} else {
		fprintf(stderr, "hvsh: command not found: %s \n", input_buffer);
	}
}

int main(void) {
	clear_buffer();
	printf("\nWelcome to HVOS %d.%d\n", HVOS_VERSION_MAJOR, HVOS_VERSION_MINOR);
	while (1) {
		printf("%s", prompt);
		index = 0;
		while (1) {
			char c;
			c = getchar();
			if (c == '\n') {
				printf("\n");
				strcpy(history_buffer[history_index++], input_buffer); 
				break;
			} else if (c == '\b') {
				if (index > 0) {
					index--;
					input_buffer[index] = '\0';
					printf("\b");
				}
			} else if (c == CLEAR) {
				clear_screen();
				clear_buffer();
				break;
			} else if (c == CANCEL) {
				clear_buffer();
				printf("\n");
				break;
			} else if(index < 127 && isgraph(c)) {
				input_buffer[index++] = c;
				printf("%c", c);
			}
		}
		if (index == 0) continue;
		exec_cmd(input_buffer);
		clear_buffer();
	}
	//exit(7);
	return 0;
}