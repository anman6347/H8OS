#include "defines.h"
#include "kozos.h"
#include "lib.h"

int test08_1_main(int argc, char *argv[]) {
    char buf[32];

    while (1) {
        puts("> ");
        gets(buf);

        if (!strncmp(buf, "echo", 4)) {
            puts(buf + 4);
            puts("\n");
        } else if (!strcmp(buf, "exit")) {
            break;
        } else {
            puts("unkonwn\n");
        }
    }

    puts("test08_1_exit.\n");

    return 0;
}