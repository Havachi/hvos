#include "hvos.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(void) {
    user_print("[INIT]: init\n");
    int pid = exec("/shell.elf");
    if (pid < 0) {
        user_print("[INIT]: Critical Error: Failed to execute shell\n");
        while (1);
    }
    waitpid(pid);
    user_print("[INIT]: Shell has exited\n");
    while (1);
    return 0;
}