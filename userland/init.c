#include "hvos.h"
#include <stdlib.h>

int main(void) {
    user_print("[INIT]: init\n");
    int pid;
    if ((pid = exec("/shell.elf")) < 0) {
        user_print("[INIT]: Critical Error: Failed to execute shell\n");
    }
    user_print("[INIT]: init done\n");
    clear_screen();
    exit(0);
}