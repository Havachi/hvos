#include "hvos.h"

void _start(void) {
    user_print("[INIT]: Starting shell\n");
    if (exec("shell.elf") < 0) {
        user_print("[INIT]: Critical Error: Failed to execute shell\n");
    }
    user_print("[INIT]: Bye bye !\n");
    exit(0);
}