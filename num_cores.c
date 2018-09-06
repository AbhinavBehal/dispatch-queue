#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>

int main(void) {
    int cores = get_nprocs();
    if (cores == 1)
        printf("This machine has %d core.\n", cores);
    else
        printf("This machine has %d cores.\n", cores);
    return EXIT_SUCCESS;
}