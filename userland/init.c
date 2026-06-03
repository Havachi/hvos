#include "hvos.h"

void _start(void) {
    user_print("[INIT]: init\n");
    if (exec("/shell.elf") < 0) {
        user_print("[INIT]: Critical Error: Failed to execute shell\n");
    }
    user_print("[INIT]: init done\n");
    clear_screen();
    exit(0);
}